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

#include "SurfaceTransaction.h"
#include "WindowUtils.h"
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "UIDriverProxy.h"
#include "uv.h"
#include "wm/InputChannel.h"
#include "wm/InputMessage.h"
#include "wm/SurfaceControl.h"
#include "wm/VsyncRequestOps.h"

namespace os {
namespace wm {

Status BaseWindow::W::moved(int32_t newX, int32_t newY) {
    // TODO
    return Status::ok();
}

Status BaseWindow::W::resized(const WindowFrames& frames, int32_t displayId) {
    // TODO
    return Status::ok();
}

Status BaseWindow::W::dispatchAppVisibility(bool visible) {
    if (mBaseWindow != nullptr) {
        mBaseWindow->dispatchAppVisibility(visible);
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
        mFrameDone(true) {
    if (mWindowManager == nullptr) {
        FLOGE("no valid window manager");
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
}

int32_t BaseWindow::getVisibility() {
    return mAppVisible ? LayoutParams::WINDOW_VISIBLE : LayoutParams::WINDOW_GONE;
}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
    if (!mAppVisible || mVsyncRequest == freq) {
        return false;
    }

    WM_PROFILER_BEGIN();

    mVsyncRequest = freq;
    mWindowManager->getService()->requestVsync(getIWindow(), freq);

    WM_PROFILER_END();
    return true;
}

void* BaseWindow::getRoot() {
    return mUIProxy.get() != nullptr ? mUIProxy->getWindow() : nullptr;
}

std::shared_ptr<BufferProducer> BaseWindow::getBufferProducer() {
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferProducer>(mSurfaceControl->bufferQueue());
    }
    FLOGI("no valid SurfaceControl when window is %svisible!", mAppVisible ? "" : "not ");
    return nullptr;
}

void BaseWindow::doDie() {
    if (mInputMonitor) {
        mInputMonitor.reset();
    }
    if (mSurfaceControl) mSurfaceControl.reset();

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

void BaseWindow::setSurfaceControl(SurfaceControl* surfaceControl) {
    /*reset current buffer when surface changed*/
    mUIProxy->resetBuffer();
    mSurfaceControl.reset(surfaceControl);

    if (surfaceControl != nullptr && surfaceControl->isValid()) {
        mUIProxy->updateResolution(surfaceControl->getWidth(), surfaceControl->getHeight(),
                                   surfaceControl->getFormat());

#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        vector<BufferId> ids;
        std::unordered_map<BufferKey, BufferId> bufferIds = mSurfaceControl->bufferIds();
        for (auto it = bufferIds.begin(); it != bufferIds.end(); ++it) {
            FLOGI("reset SurfaceControl bufferId:%s,%d", it->second.mName.c_str(), it->second.mKey);

            int32_t fd = shm_open(it->second.mName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
            ids.push_back({it->second.mName, it->second.mKey, fd});
        }
        mSurfaceControl->initBufferIds(ids);
#endif
    }
}

void BaseWindow::dispatchAppVisibility(bool visible) {
    WM_PROFILER_BEGIN();

    FLOGI("%p visible:%d", this, visible);
    handleAppVisibility(visible);
    WM_PROFILER_END();
}

void BaseWindow::onFrame(int32_t seq) {
    WM_PROFILER_BEGIN();
    if (!mFrameDone.load(std::memory_order_acquire)) {
        FLOGD("onFrame(%p) %d, waiting frame done!", this, seq);
        WM_PROFILER_END();
        return;
    }

    mFrameDone.exchange(false, std::memory_order_release);
    WM_PROFILER_END();
    handleOnFrame(seq);
    mFrameDone.exchange(true, std::memory_order_release);
}

void BaseWindow::bufferReleased(int32_t bufKey) {
    WM_PROFILER_BEGIN();
    FLOGD("bufKey:%d", bufKey);
    handleBufferReleased(bufKey);
    WM_PROFILER_END();
}

void BaseWindow::handleAppVisibility(bool visible) {
    FLOGI("visible from %d to %d", mAppVisible, visible);

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
        // release mSurfaceControl
        mSurfaceControl.reset();
    }

    if (!mAppVisible) {
        mVsyncRequest = VsyncRequest::VSYNC_REQ_NONE;
    } else {
        scheduleVsync(VsyncRequest::VSYNC_REQ_SINGLE);
    }

    WM_PROFILER_END();
}

void BaseWindow::handleOnFrame(int32_t seq) {
    if (!mAppVisible) {
        FLOGD("window needn't update.");
        return;
    }

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    FLOGD("frame(%p) %d", this, seq);

    if (mSurfaceControl.get() == nullptr) {
        mWindowManager->relayoutWindow(shared_from_this());
        if (mSurfaceControl->isValid()) {
            updateOrCreateBufferQueue();
        }
    } else {
        if (mUIProxy.get() == nullptr) {
            FLOGI("UIProxy is invalid!");
            return;
        }

        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
        if (buffProducer.get() == nullptr) {
            FLOGI("buffProducer is invalid!");
            return;
        }
        BufferItem* item = buffProducer->dequeueBuffer();
        if (!item) {
            FLOGI("onFrame, no valid buffer!\n");
            return;
        }

        WM_PROFILER_BEGIN();
        mUIProxy->drawFrame(item);
        WM_PROFILER_END();
        if (!mUIProxy->finishDrawing()) {
            FLOGD("no finish drawing!");
            buffProducer->cancelBuffer(item);
            return;
        }
        buffProducer->queueBuffer(item);

        auto transaction = mWindowManager->getTransaction();
        transaction->setBuffer(mSurfaceControl, *item);
        auto rect = mUIProxy->rectCrop();
        if (rect) transaction->setBufferCrop(mSurfaceControl, *rect);

        FLOGD("frame(%p) %d apply transaction\n", this, seq);
        transaction->apply();

        WindowEventListener* listener = mUIProxy->getEventListener();
        if (listener) {
            listener->onPostDraw();
        }
    }
}

void BaseWindow::handleBufferReleased(int32_t bufKey) {
    std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
    if (buffProducer.get() == nullptr) {
        return;
    }

    WM_PROFILER_BEGIN();
    auto buffer = buffProducer->syncFreeState(bufKey);
    if (!buffer) {
        FLOGD("bufferReleased, release %d failure!", bufKey);
    }
    WM_PROFILER_END();
    FLOGD("release bufKey:%d done!", bufKey);
}

void BaseWindow::updateOrCreateBufferQueue() {
    if (mSurfaceControl->bufferQueue() != nullptr) {
        mSurfaceControl->bufferQueue()->update(mSurfaceControl);
    } else {
        std::shared_ptr<BufferProducer> buffProducer =
                std::make_shared<BufferProducer>(mSurfaceControl);
        mSurfaceControl->setBufferQueue(buffProducer);
    }
    FLOGI("updateOrCreateBufferQueue done!");
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

} // namespace wm
} // namespace os
