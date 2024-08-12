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

#include "BaseWindow.h"

#include <mqueue.h>

#include "../common/FrameTimeInfo.h"
#include "../common/WindowUtils.h"
#include "SurfaceTransaction.h"
#include "UIDriverProxy.h"
#include "uv.h"
#include "wm/InputChannel.h"
#include "wm/InputMessage.h"
#include "wm/SurfaceControl.h"
#include "wm/VsyncRequestOps.h"

namespace os {
namespace wm {

Status BaseWindow::W::moved(int32_t newX, int32_t newY) {
    return Status::ok();
}

Status BaseWindow::W::resized(const WindowFrames& frames, int32_t displayId) {
    return Status::ok();
}

Status BaseWindow::W::dispatchAppVisibility(bool visible) {
    if (mBaseWindow != nullptr) {
        mBaseWindow->setVisible(visible);
    }
    return Status::ok();
}

Status BaseWindow::W::onFrame(int32_t seq) {
    if (mBaseWindow != nullptr) {
        mBaseWindow->onFrame(seq);
    }
    return Status::ok();
}

Status BaseWindow::W::bufferReleased(int32_t bufKey) {
    if (mBaseWindow != nullptr) {
        mBaseWindow->bufferReleased(bufKey);
    }
    return Status::ok();
}

void BaseWindow::W::clear() {
    mBaseWindow = nullptr;
}

BaseWindow::~BaseWindow() {
    doDie();
    mIWindow = nullptr;
}

BaseWindow::BaseWindow(::os::app::Context* context, WindowManager* wm)
      : mContext(context),
        mWindowManager(wm),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mAppVisible(false),
        mFrameDone(true),
        mSurfaceBufferReady(false),
        mTraceFrame(false),
        mFrameTimeInfo(nullptr) {
    if (mWindowManager == nullptr) {
        FLOGE("%p no valid window manager", this);
        return;
    }

    uint32_t width = 0, height = 0;

    mInputMonitor = std::make_shared<InputMonitor>();
    mAttrs = LayoutParams();

    mWindowManager->getDisplayInfo(&width, &height);
    mAttrs.mWidth = width;
    mAttrs.mHeight = height;

    mAttrs.mToken = context->getToken();
    mIWindow = sp<W>::make(this);
    mFrameTimeInfo = new FrameTimeInfo();
}

int32_t BaseWindow::getVisibility() {
    return mAppVisible ? LayoutParams::WINDOW_VISIBLE : LayoutParams::WINDOW_GONE;
}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
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
    FLOGD("%p request vreq=%s", this, VsyncRequestToString(mVsyncRequest));
    mWindowManager->getService()->requestVsync(getIWindow(), mVsyncRequest);

    WM_PROFILER_END();
    return true;
}

void* BaseWindow::getNativeDisplay() {
    return mUIProxy.get() != nullptr ? mUIProxy->getRoot() : nullptr;
}

void BaseWindow::setUIProxy(const std::shared_ptr<UIDriverProxy>& proxy) {
    mUIProxy = proxy;
    mUIProxy->traceFrame(mTraceFrame);
}

void* BaseWindow::getRoot() {
    return mUIProxy.get() != nullptr ? mUIProxy->getWindow() : nullptr;
}

std::shared_ptr<BufferProducer> BaseWindow::getBufferProducer() {
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferProducer>(mSurfaceControl->bufferQueue());
    }
    FLOGI("%p no valid SurfaceControl when window is %svisible!", this, mAppVisible ? "" : "not ");
    return nullptr;
}

void BaseWindow::doDie() {
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
    mIWindow->clear();
}

void BaseWindow::setInputChannel(InputChannel* inputChannel) {
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

void BaseWindow::clearSurfaceBuffer() {
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    /*destroy current sc buffers */
    if (mSurfaceBufferReady) {
        uninitSurfaceBuffer(mSurfaceControl);
        mSurfaceBufferReady = false;
    }
#endif
}

void BaseWindow::setSurfaceControl(SurfaceControl* surfaceControl) {
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

void BaseWindow::onFrame(int32_t seq) {
    WM_PROFILER_BEGIN();

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    FLOGD("%p frame seq=%" PRIu32 "", this, seq);

    if (mUIProxy.get() == nullptr) {
        FLOGE("%p frame seq=%" PRIu32 ", ui proxy exception", this, seq);
        return;
    }

    /* mark vsync */
    auto info = mUIProxy->frameMetaInfo();
    if (info) info->setVsync(FrameMetaInfo::getCurSysTime(), seq, mUIProxy->getTimerPeriod());
    mUIProxy->notifyVsyncEvent();

    if (!mFrameDone.load(std::memory_order_acquire)) {
        FLOGD("%p frame seq=%" PRIu32 ", waiting frame done!", this, seq);
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
            FLOGI("SingleFrameLog{seq=%" PRIu32 ", skip=%d}", seq, (int)(*skipReason));
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

void BaseWindow::setVisible(bool visible) {
    FLOGI("%p visible from %d to %d", this, mAppVisible, visible);

    if (visible == mAppVisible) {
        return;
    }
    WM_PROFILER_BEGIN();

    mAppVisible = visible;
    mUIProxy->updateVisibility(mAppVisible);

    mWindowManager->relayoutWindow(shared_from_this());
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        mSurfaceControl.reset();
    }

    if (!mAppVisible) {
        mVsyncRequest = VsyncRequest::VSYNC_REQ_NONE;
        FLOGI("%p window is hidden, reset vreq to none.", this);
    } else {
        scheduleVsync(VsyncRequest::VSYNC_REQ_SINGLE);
    }

    WM_PROFILER_END();
}

void BaseWindow::handleOnFrame(int32_t seq) {
    auto info = mUIProxy->frameMetaInfo();

    if (!mAppVisible) {
        FLOGD("%p window needn't update.", this);
        if (info) info->setSkipReason(FrameMetaSkipReason::NoSurface);
        return;
    }

    if (mSurfaceControl.get() == nullptr) {
        if (info) info->setSkipReason(FrameMetaSkipReason::NoSurface);
        mWindowManager->relayoutWindow(shared_from_this());
        if (mSurfaceControl.get() && mSurfaceControl->isValid()) {
            updateOrCreateBufferQueue();
        }
    } else {
        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
        if (buffProducer.get() == nullptr) {
            FLOGI("%p seq=%" PRIu32 " buffProducer is invalid!", this, seq);
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
            FLOGI("%p seq=%" PRIu32 " no valid buffer!\n", this, seq);
            if (info) info->setSkipReason(FrameMetaSkipReason::NoBuffer);
            return;
        }

        WM_PROFILER_BEGIN();
        mUIProxy->drawFrame(item);
        WM_PROFILER_END();
        if (!mUIProxy->finishDrawing()) {
            FLOGI("%p seq=%" PRIu32 " no valid drawing!", this, seq);
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

        FLOGI("%p seq=%" PRIu32 " apply frame transaction\n", this, seq);
        transaction->apply();

        WindowEventListener* listener = mUIProxy->getEventListener();
        if (listener) {
            listener->onPostDraw();
        }
    }
}

void BaseWindow::bufferReleased(int32_t bufKey) {
    std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
    if (buffProducer.get() == nullptr) {
        return;
    }

    WM_PROFILER_BEGIN();
    auto buffer = buffProducer->syncFreeState(bufKey);
    if (!buffer) {
        FLOGD("%p bufferReleased, release %" PRId32 " failure!", this, bufKey);
    }
    WM_PROFILER_END();
    FLOGI("%p release bufKey:%" PRId32 " done!\n", this, bufKey);
}

void BaseWindow::updateOrCreateBufferQueue() {
    if (mSurfaceControl->bufferQueue() != nullptr) {
        mSurfaceControl->bufferQueue()->update(mSurfaceControl);
    } else {
        std::shared_ptr<BufferProducer> buffProducer =
                std::make_shared<BufferProducer>(mSurfaceControl);
        mSurfaceControl->setBufferQueue(buffProducer);
    }
    FLOGI("%p updateOrCreateBufferQueue done!", this);
}

void BaseWindow::setEventListener(WindowEventListener* listener) {
    if (mUIProxy && mUIProxy.get()) mUIProxy->setEventListener(listener);
}

void BaseWindow::setLayoutParams(LayoutParams lp) {
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

void BaseWindow::setType(int32_t type) {
    mAttrs.mType = type;
}

void BaseWindow::traceFrame(bool enable) {
    mTraceFrame = enable;
    if (mUIProxy.get()) {
        mUIProxy->traceFrame(enable);
    }
}

} // namespace wm
} // namespace os
