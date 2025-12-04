/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "BaseWindow"

#include <mqueue.h>

#include "../common/FrameTimeInfo.h"
#include "../common/WindowUtils.h"
#include "BaseWindowDefault.h"
#include "SurfaceTransaction.h"
#include "UIDriverProxy.h"
#include "uv.h"
#include "wm/InputChannel.h"
#include "wm/InputMessage.h"
#include "wm/SurfaceControl.h"
#include "wm/VsyncRequestOps.h"
#include "wm/WindowFrames.h"

namespace os {
namespace wm {

Status BaseWindowDefault::W::moved(int32_t newX, int32_t newY) {
    return Status::ok();
}

Status BaseWindowDefault::W::resized(const WindowFrames& frames, int32_t displayId) {
    return Status::ok();
}

Status BaseWindowDefault::W::dispatchAppVisibility(bool visible) {
    if (mBaseWindow != nullptr) {
        mBaseWindow->setVisible(visible);
    }
    return Status::ok();
}

Status BaseWindowDefault::W::onFrame(int32_t seq) {
    if (!xmsLiteMode() && mBaseWindow != nullptr) {
        mBaseWindow->onFrame(seq);
    }
    return Status::ok();
}

Status BaseWindowDefault::W::bufferReleased(int32_t bufKey) {
    if (!xmsLiteMode() && mBaseWindow != nullptr) {
        mBaseWindow->bufferReleased(bufKey);
    }
    return Status::ok();
}

void BaseWindowDefault::W::clear() {
    mBaseWindow = nullptr;
}

BaseWindowDefault::~BaseWindowDefault() {
    doDie();
    mIWindow = nullptr;
    mRoot = nullptr;
}

BaseWindowDefault::BaseWindowDefault(::os::app::Context* context, WindowManagerDefault* wm)
      : mContext(context),
        mWindowManager(wm),
        mAppVisible(false),
        mRoot(nullptr),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mFrameDone(true),
        mSurfaceBufferReady(false),
        mTraceFrame(false),
        mFrameTimeInfo(nullptr) {
    if (mWindowManager == nullptr) {
        FLOGE("no valid window manager");
        return;
    }
    uint32_t width = 0, height = 0;
    mWindowManager->getDisplayInfo(&width, &height);

    mAttrs.mWidth = width;
    mAttrs.mHeight = height;
    mAttrs.mToken = context->getToken();
    FLOGI("window size (%" PRId32 "x%" PRId32 ")", mAttrs.mWidth, mAttrs.mHeight);
    mIWindow = sp<W>::make(this);

    if (xmsLiteMode()) return;

    mInputMonitor = std::make_shared<InputMonitor>();
    mFrameTimeInfo = new FrameTimeInfo();
}

bool BaseWindowDefault::onCreateStage() {
    if (mWindowManager == nullptr) {
        return false;
    }

    return mWindowManager->attachIWindow(shared_from_this()) == 0 ? true : false;
}

bool BaseWindowDefault::onDestroyStage() {
    return mWindowManager == nullptr ? false : mWindowManager->removeWindow(shared_from_this());
}

int32_t BaseWindowDefault::getVisibility() {
    return mAppVisible ? LayoutParams::WINDOW_VISIBLE : LayoutParams::WINDOW_GONE;
}

void BaseWindowDefault::doDie() {
    mIWindow->clear();

    if (xmsLiteMode()) {
        return;
    }

    if (mInputMonitor) {
        mInputMonitor.reset();
    }
    if (mSurfaceControl) {
        clearSurfaceBuffer();
        mSurfaceControl.reset();
    }

    if (mFrameTimeInfo) {
        delete static_cast<FrameTimeInfo*>(mFrameTimeInfo);
        mFrameTimeInfo = NULL;
    }
    mUIProxy.reset();
}

void BaseWindowDefault::setVisible(bool visible) {
    FLOGI("visible from %s to %s", mAppVisible ? "visible" : "invisible",
          visible ? "visible" : "invisible");

    if (visible == mAppVisible) {
        return;
    }
    WM_PROFILER_BEGIN();

    mAppVisible = visible;
    if (!xmsLiteMode()) mUIProxy->updateVisibility(mAppVisible);

    mWindowManager->relayoutWindow(shared_from_this());

    if (xmsLiteMode()) {
        WM_PROFILER_END();
        return;
    }

    /*========== only for multi-instance mode ==========*/
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        mSurfaceControl.reset();
    }

    if (!mAppVisible) {
        mVsyncRequest = VsyncRequest::VSYNC_REQ_NONE;
        FLOGI("window is hidden, reset vreq to none.");
    } else {
        scheduleVsync(VsyncRequest::VSYNC_REQ_SINGLE);
    }

    WM_PROFILER_END();
}

void BaseWindowDefault::setLayoutParams(LayoutParams lp) {
    mAttrs = lp;

    if (mWindowManager) {
        uint32_t width = 0, height = 0;
        mWindowManager->getDisplayInfo(&width, &height);

        if (mAttrs.mWidth < 0) {
            mAttrs.mWidth = width;
        } else {
            mAttrs.mWidth = DATA_CLAMP(mAttrs.mWidth, 0, (int32_t)width * 2);
        }

        if (mAttrs.mHeight < 0) {
            mAttrs.mHeight = height;
        } else {
            mAttrs.mHeight = DATA_CLAMP(mAttrs.mHeight, 0, (int32_t)height * 2);
        }
    }
}

void BaseWindowDefault::setType(int32_t type) {
    mAttrs.mType = type;
}

void* BaseWindowDefault::getRoot() {
    if (xmsLiteMode()) return mRoot;

    if (mRoot == nullptr) {
        mRoot = mUIProxy.get() != nullptr ? mUIProxy->getWindow() : nullptr;
    }
    return mRoot;
}

void BaseWindowDefault::initRoot(void* root) {
    /* only for lite mode */
    if (!xmsLiteMode()) return;

    mRoot = root;
    FLOGI("root %s", root != nullptr ? "valid" : "invalid");
}

/*========== only for multi-instance mode ==========*/
bool BaseWindowDefault::scheduleVsync(VsyncRequest freq) {
    if (xmsLiteMode()) return false;

    if (!mAppVisible) {
        return false;
    }

    auto newfreq = (mUIProxy.get() && mUIProxy->vsyncEventEnabled())
            ? VsyncRequest::VSYNC_REQ_PERIODIC
            : freq;
    if (mVsyncRequest == newfreq) {
        return false;
    }

    WM_PROFILER_BEGIN();

    if (mUIProxy->frameMetaInfo() && mFrameTimeInfo &&
        (newfreq == VsyncRequest::VSYNC_REQ_PERIODIC ||
         mVsyncRequest == VsyncRequest::VSYNC_REQ_PERIODIC)) {
        static_cast<FrameTimeInfo*>(mFrameTimeInfo)->time(NULL);
    }

    mVsyncRequest = newfreq;
    FLOGD("request vreq=%s", VsyncRequestToString(mVsyncRequest));
    mWindowManager->getService()->requestVsync(getIWindow(), mVsyncRequest);

    WM_PROFILER_END();
    return true;
}

void* BaseWindowDefault::getNativeDisplay() {
    if (xmsLiteMode()) return nullptr;
    return mUIProxy.get() != nullptr ? mUIProxy->getRoot() : nullptr;
}

void BaseWindowDefault::initUIProxy(const std::shared_ptr<UIDriverProxy>& proxy) {
    if (xmsLiteMode()) return;
    mUIProxy = proxy;
    mUIProxy->traceFrame(mTraceFrame);
}

std::shared_ptr<BufferProducer> BaseWindowDefault::getBufferProducer() {
    if (xmsLiteMode()) return nullptr;
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferProducer>(mSurfaceControl->bufferQueue());
    }
    FLOGI("no valid SurfaceControl when window is %svisible!", mAppVisible ? "" : "not ");
    return nullptr;
}

void BaseWindowDefault::setInputChannel(InputChannel* inputChannel) {
    if (xmsLiteMode()) return;

    if (inputChannel != nullptr && inputChannel->isValid()) {
        mInputMonitor->setInputChannel(inputChannel);
        mUIProxy->setInputMonitor(mInputMonitor.get());
        mInputMonitor->start(mContext->getMainLoop()->get(),
                             [this](InputMonitor* monitor) { mUIProxy->handleEvent(); });
    } else if (mInputMonitor && mInputMonitor->isValid()) {
        mUIProxy->setInputMonitor(nullptr);
        mInputMonitor.reset();
    }
}

void BaseWindowDefault::clearSurfaceBuffer() {
    if (xmsLiteMode()) return;
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    /*destroy current sc buffers */
    if (mSurfaceBufferReady) {
        uninitSurfaceBuffer(mSurfaceControl);
        mSurfaceBufferReady = false;
    }
#endif
}

void BaseWindowDefault::setSurfaceControl(SurfaceControl* surfaceControl) {
    if (xmsLiteMode()) return;

    /*reset current buffer when surface changed*/
    mUIProxy->resetBuffer();

    clearSurfaceBuffer();
    if (surfaceControl == nullptr)
        mSurfaceControl.reset();
    else
        mSurfaceControl.reset(surfaceControl);

    if (surfaceControl != nullptr && surfaceControl->isValid()) {
        mUIProxy->updateResolution(surfaceControl->getWidth(), surfaceControl->getHeight(),
                                   surfaceControl->getFormat());
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        initSurfaceBuffer(mSurfaceControl, false);
        mSurfaceBufferReady = true;
#endif
    }
}

void BaseWindowDefault::onFrame(int32_t seq) {
    if (xmsLiteMode()) return;
    WM_PROFILER_BEGIN();

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    FLOGD("frame seq=%" PRIu32 "", seq);

    if (mSurfaceControl.get()) {
        FLOGD("update response seq=%" PRIu32 "", seq);
        mSurfaceControl->getFMQ().updateClientRespSeq(seq);
    }

    if (mUIProxy.get() == nullptr) {
        FLOGE("frame seq=%" PRIu32 ", ui proxy exception", seq);
        return;
    }

    if (mVsyncRequest == VsyncRequest::VSYNC_REQ_PERIODIC && !mUIProxy->needPeriodicVsync()) {
        FLOGI("frame seq=%" PRIu32 ", stop periodic vsync automatically.", seq);
        scheduleVsync(VsyncRequest::VSYNC_REQ_SINGLE);
    }

    /* mark vsync */
    auto info = mUIProxy->frameMetaInfo();
    if (info) info->setVsync(FrameMetaInfo::getCurSysTime(), seq, mUIProxy->getTimerPeriod());
    mUIProxy->notifyVsyncEvent();

    if (!mFrameDone.load(std::memory_order_acquire)) {
        FLOGD("frame seq=%" PRIu32 ", waiting frame done!", seq);
        if (info) info->setSkipReason(FrameMetaSkipReason::NoTarget);
        WM_PROFILER_END();
        return;
    }

    /* mark draw start*/
    if (info) info->markFrameStart();

    mFrameDone.exchange(false, std::memory_order_release);
    WM_PROFILER_END();
    handleOnFrame(seq);
    mFrameDone.exchange(true, std::memory_order_release);

    if (info) {
        /* mark frame finished*/
        info->markFrameFinished();

        auto skipReason = info->getSkipReason();
        if (skipReason) {
            /* invalid sample */
            FLOGD("SingleFrameLog{seq=%" PRIu32 ", skip=%d}", seq, (int)(*skipReason));
        } else {
            FLOGW("SingleFrameLog{seq=%" PRIu32 ", totalMs=%" PRId64 ", animMs=%" PRId64
                  ", renderMs=%" PRId64 ", layoutMs=%" PRId64 ", transactMs=%" PRId64 "}",
                  seq, info->totalDuration(), info->totalVsyncDuration(),
                  info->totalRenderDuration(), info->totalLayoutDuration(),
                  info->totalTransactDuration());
        }
        if (mFrameTimeInfo) static_cast<FrameTimeInfo*>(mFrameTimeInfo)->time(info);
    }
}

void BaseWindowDefault::handleOnFrame(int32_t seq) {
    if (xmsLiteMode()) return;
    auto info = mUIProxy->frameMetaInfo();

    if (!mAppVisible) {
        FLOGD("window needn't update.");
        if (info) info->setSkipReason(FrameMetaSkipReason::NoSurface);
        return;
    }

    if (mSurfaceControl.get() == nullptr) {
        if (info) info->setSkipReason(FrameMetaSkipReason::NoSurface);
        mWindowManager->relayoutWindow(shared_from_this());
        if (mSurfaceControl.get() && mSurfaceControl->isValid()) {
            updateOrCreateBufferQueue();
            FLOGD("firstly, update response seq=%" PRIu32 "", seq);
            mSurfaceControl->getFMQ().updateClientRespSeq(seq);
        }
    } else {
        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
        if (buffProducer.get() == nullptr) {
            FLOGI("seq=%" PRIu32 " buffProducer is invalid!", seq);
            if (info) info->setSkipReason(FrameMetaSkipReason::NoBuffer);
            return;
        }

        BufferKey key;
        if (mSurfaceControl->getFMQ().read(&(key))) {
            bufferReleased(key);
        }

        BufferItem* item = buffProducer->dequeueBuffer();
        if (!item) {
            if (mVsyncRequest != VsyncRequest::VSYNC_REQ_PERIODIC)
                scheduleVsync(VsyncRequest::VSYNC_REQ_SINGLESUPPRESS);
            FLOGD("seq=%" PRIu32 " no valid buffer!\n", seq);
            if (info) info->setSkipReason(FrameMetaSkipReason::NoBuffer);
            return;
        }

        WM_PROFILER_BEGIN();
        mUIProxy->drawFrame(item);
        WM_PROFILER_END();
        if (!mUIProxy->finishDrawing()) {
            FLOGD("seq=%" PRIu32 " no valid drawing!", seq);
            buffProducer->cancelBuffer(item);
            if (info) info->setSkipReason(FrameMetaSkipReason::NothingToDraw);
            return;
        }
        if (info) info->markSyncQueued();
        buffProducer->queueBuffer(item);

        auto transaction = mWindowManager->getTransaction();
        transaction->setBuffer(mSurfaceControl, *item, seq);
        auto rect = mUIProxy->rectCrop();
        if (rect) transaction->setBufferCrop(mSurfaceControl, *rect);

        FLOGD("seq=%" PRIu32 " apply frame transaction\n", seq);
        transaction->apply();

        WindowEventListener* listener = mUIProxy->getEventListener();
        if (listener) {
            listener->onPostDraw();
        }
    }
}

void BaseWindowDefault::bufferReleased(int32_t bufKey) {
    if (xmsLiteMode()) return;
    std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
    if (buffProducer.get() == nullptr) {
        return;
    }

    WM_PROFILER_BEGIN();
    auto buffer = buffProducer->syncFreeState(bufKey);
    if (!buffer) {
        FLOGD("bufferReleased, release %" PRId32 " failure!", bufKey);
    }
    WM_PROFILER_END();
    FLOGD("release bufKey:%" PRId32 " done!\n", bufKey);
}

void BaseWindowDefault::updateOrCreateBufferQueue() {
    if (xmsLiteMode()) return;
    if (mSurfaceControl->bufferQueue() != nullptr) {
        mSurfaceControl->bufferQueue()->update(mSurfaceControl);
    } else {
        std::shared_ptr<BufferProducer> buffProducer =
                std::make_shared<BufferProducer>(mSurfaceControl);
        mSurfaceControl->setBufferQueue(buffProducer);
    }
    FLOGI("done");
}

void BaseWindowDefault::setEventListener(WindowEventListener* listener) {
    if (mUIProxy && mUIProxy.get()) mUIProxy->setEventListener(listener);
}

void BaseWindowDefault::traceFrame(bool enable) {
    mTraceFrame = enable;
    if (mUIProxy.get()) {
        mUIProxy->traceFrame(enable);
    }
}

} // namespace wm
} // namespace os
