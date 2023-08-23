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

#include "LogUtils.h"
#include "SurfaceTransaction.h"
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "../system_server/BaseProfiler.h"
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

static void eventCallback(int fd, int status, int events, void* data) {
    if (status < 0) {
        FLOGE("Poll error: %s ", uv_strerror(status));
        return;
    }

    if (events & UV_READABLE) {
        InputMessage msg;
        ssize_t size = mq_receive(fd, (char*)&msg, sizeof(InputMessage), NULL);
        if (size != sizeof(InputMessage)) {
            return;
        }

        BaseWindow* win = reinterpret_cast<BaseWindow*>(data);
        if (win) {
            auto proxy = win->getUIProxy();
            if (proxy.get() != nullptr) proxy->handleEvent(msg);
        }
    }
}

bool BaseWindow::readEvent(InputMessage* message) {
    if (mEventFd < 0 || message == NULL) return false;

    ssize_t size = mq_receive(mEventFd, (char*)message, sizeof(InputMessage), NULL);
    return size == sizeof(InputMessage) ? true : false;
}

BaseWindow::~BaseWindow() {
    if (mPoll) {
        uv_poll_stop(mPoll);
        uv_close(reinterpret_cast<uv_handle_t*>(mPoll),
                 [](uv_handle_t* handle) { delete reinterpret_cast<uv_poll_t*>(handle); });
        mPoll = NULL;
    }
}

BaseWindow::BaseWindow(::os::app::Context* context)
      : mContext(context),
        mWindowManager(nullptr),
        mPoll(nullptr),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mVisible(false),
        mFrameDone(true) {
    mAttrs = LayoutParams();
    mAttrs.mToken = context->getToken();

    mIWindow = sp<W>::make(this);
}

void BaseWindow::setWindowManager(WindowManager* wm) {
    mWindowManager = wm;
}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
    if (mVsyncRequest == freq) {
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
    FLOGI("no valid SurfaceControl when window is %svisible!", isVisible() ? "" : "not ");
    return nullptr;
}

void BaseWindow::doDie() {
    setInputChannel(nullptr);
    mSurfaceControl.reset();
    mUIProxy.reset();
}

void BaseWindow::setInputChannel(InputChannel* inputChannel) {
    if (inputChannel != nullptr && inputChannel->isValid()) {
        mInputChannel.reset(inputChannel);
        mEventFd = mInputChannel->getEventFd();
        if (!mUIProxy->enableInput(true)) {
            mPoll = new uv_poll_t;
            mPoll->data = this;
            uv_poll_init(mContext->getMainLoop()->get(), mPoll, mEventFd);
            uv_poll_start(mPoll, UV_READABLE, [](uv_poll_t* handle, int status, int events) {
                eventCallback(handle->io_watcher.fd, status, events, handle->data);
            });
        }
    } else if (mInputChannel && mInputChannel->isValid()) {
        mEventFd = -1;
        mInputChannel.reset();
    }
}

void BaseWindow::setSurfaceControl(SurfaceControl* surfaceControl) {
    mSurfaceControl.reset(surfaceControl);
    if (surfaceControl) {
        mUIProxy->updateResolution(surfaceControl->getWidth(), surfaceControl->getHeight());
    }

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

void BaseWindow::dispatchAppVisibility(bool visible) {
    WM_PROFILER_BEGIN();
    FLOGI("visible:%d", visible);
    mContext->getMainLoop()->postTask(
            [this, visible](void*) { this->handleAppVisibility(visible); });
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

    mContext->getMainLoop()->postTask([this, seq](void*) {
        this->handleOnFrame(seq);
        mFrameDone.exchange(true, std::memory_order_release);
    });
    WM_PROFILER_END();
}

void BaseWindow::bufferReleased(int32_t bufKey) {
    WM_PROFILER_BEGIN();
    FLOGD("bufKey:%d", bufKey);
    mContext->getMainLoop()->postTask(
            [this, bufKey](void*) { this->handleBufferReleased(bufKey); });
    WM_PROFILER_END();
}

void BaseWindow::handleAppVisibility(bool visible) {
    FLOGI("visible from %d to %d", mVisible, visible);

    if (mVisible == visible) {
        return;
    }

    WM_PROFILER_BEGIN();
    mVisible = visible;
    mWindowManager->relayoutWindow(shared_from_this());
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        // release mSurfaceControl
        mSurfaceControl.reset();
    }

    if (!visible) {
        mVsyncRequest = VsyncRequest::VSYNC_REQ_NONE;
    }

    WM_PROFILER_END();
}

void BaseWindow::handleOnFrame(int32_t seq) {
    if (!isVisible()) {
        FLOGD("window is not visible now.");
        return;
    }

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    FLOGD("frame(%p) %d", this, seq);

    if (mSurfaceControl.get() == nullptr) {
        WM_PROFILER_BEGIN();
        mWindowManager->relayoutWindow(shared_from_this());
        if (mSurfaceControl->isValid()) {
            updateOrCreateBufferQueue();
        }
        WM_PROFILER_END();
    } else {
        if (mUIProxy.get() == nullptr) {
            return;
        }

        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
        if (buffProducer.get() == nullptr) {
            FLOGD("buffProducer is invalid!");
            WM_PROFILER_END();
            return;
        }
        BufferItem* item = buffProducer->dequeueBuffer();
        if (!item) {
            FLOGD("onFrame, no valid buffer!\n");
            WM_PROFILER_END();
            return;
        }

        WM_PROFILER_BEGIN();
        mUIProxy->drawFrame(item);
        if (!mUIProxy->finishDrawing()) {
            buffProducer->cancelBuffer(item);
            WM_PROFILER_END();
            return;
        }
        buffProducer->queueBuffer(item);

        auto transaction = mWindowManager->getTransaction();
        transaction->setBuffer(mSurfaceControl, *item);
        auto rect = mUIProxy->rectCrop();
        if (rect) transaction->setBufferCrop(mSurfaceControl, *rect);

        FLOGD("frame(%p) %d apply transaction\n", this, seq);
        transaction->apply();

        auto callback = mUIProxy->getEventCallback();
        if (callback) {
            callback(this, 0, MOCKUI_EVENT_POSTDRAW);
        }
        WM_PROFILER_END();
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

void BaseWindow::setMockUIEventCallback(const MOCKUI_EVENT_CALLBACK& cb) {
    if (mUIProxy.get() == nullptr) return;
    mUIProxy->setEventCallback(cb);
}

} // namespace wm
} // namespace os
