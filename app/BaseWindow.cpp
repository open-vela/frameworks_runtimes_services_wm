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

#include "BaseWindow.h"

#include <mqueue.h>

#include "SurfaceTransaction.h"
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

static void eventCallback(int fd, int status, int events, void* data) {
    if (status < 0) {
        ALOGE("Poll error: %s ", uv_strerror(status));
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

BaseWindow::~BaseWindow() {
    if (mPoll) delete mPoll;
}

BaseWindow::BaseWindow(::os::app::Context* context)
      : mContext(context),
        mWindowManager(nullptr),
        mPoll(nullptr),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mAppVisible(false) {
    mAttrs = LayoutParams();
    mAttrs.mToken = context->getToken();

    mIWindow = sp<W>::make(this);
}

void BaseWindow::setWindowManager(WindowManager* wm) {
    mWindowManager = wm;
}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
    if (mVsyncRequest == freq) {
        ALOGI("Warning: It's waiting previous vsync response.");
        return false;
    }

    mVsyncRequest = freq;
    mWindowManager->getService()->requestVsync(getIWindow(), freq);
    return true;
}

void* BaseWindow::getRoot() {
    return mUIProxy.get() != nullptr ? mUIProxy->getWindow() : nullptr;
}

std::shared_ptr<BufferProducer> BaseWindow::getBufferProducer() {
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferProducer>(mSurfaceControl->bufferQueue());
    }
    ALOGW("mSurfaceControl is not existed");
    return nullptr;
}

void BaseWindow::doDie() {
    // TODO destory lvglDriver
    mInputChannel.reset();
    mSurfaceControl.reset();
    delete mPoll;
}

void BaseWindow::setInputChannel(InputChannel* inputChannel) {
    if (inputChannel != nullptr && inputChannel->isValid()) {
        mInputChannel.reset(inputChannel);
        mPoll = new ::os::app::UvPoll(mContext->getMainLoop(), mInputChannel->getEventFd());
        mPoll->start(UV_READABLE, eventCallback, this);
    } else {
        mInputChannel.reset();
    }
}

void BaseWindow::setSurfaceControl(SurfaceControl* surfaceControl) {
    mSurfaceControl.reset(surfaceControl);

#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    vector<BufferId> ids;
    std::unordered_map<BufferKey, BufferId> bufferIds = mSurfaceControl->bufferIds();
    for (auto it = bufferIds.begin(); it != bufferIds.end(); ++it) {
        ALOGI("reset SurfaceControl bufferId:%s,%d", it->second.mName.c_str(), it->second.mKey);

        int32_t fd = shm_open(it->second.mName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
        ids.push_back({it->second.mName, it->second.mKey, fd});
    }
    mSurfaceControl->initBufferIds(ids);
#endif
}

void BaseWindow::dispatchAppVisibility(bool visible) {
    mContext->getMainLoop()->postTask(
            [this, visible](void*) { this->handleAppVisibility(visible); });
}

void BaseWindow::onFrame(int32_t seq) {
    mContext->getMainLoop()->postTask([this, seq](void*) { this->handleOnFrame(seq); });
}

void BaseWindow::bufferReleased(int32_t bufKey) {
    mContext->getMainLoop()->postTask(
            [this, bufKey](void*) { this->handleBufferReleased(bufKey); });
}

void BaseWindow::handleAppVisibility(bool visible) {
    if (mAppVisible == visible) {
        return;
    }
    mAppVisible = visible;
    mWindowManager->relayoutWindow(shared_from_this());
    if (mSurfaceControl.get() != nullptr && mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        // release mSurfaceControl
        mSurfaceControl.reset();
    }
}

void BaseWindow::handleOnFrame(int32_t seq) {
    mVsyncRequest = nextVsyncState(mVsyncRequest);

    if (mSurfaceControl.get() == nullptr) {
        mWindowManager->relayoutWindow(shared_from_this());
        if (mSurfaceControl->isValid()) {
            updateOrCreateBufferQueue();
        }
    } else {
        if (mUIProxy.get() == nullptr) return;

        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
        if (buffProducer.get() == nullptr) {
            ALOGW("buffProducer is not existed!");
            return;
        }
        BufferItem* item = buffProducer->dequeueBuffer();
        if (!item) {
            ALOGI("onFrame, no valid buffer!");
            return;
        }

        mUIProxy->drawFrame(item);
        if (!mUIProxy->finishDrawing()) {
            buffProducer->cancelBuffer(item);
            return;
        }

        buffProducer->queueBuffer(item);

        auto transaction = mWindowManager->getTransaction();
        transaction->setBuffer(mSurfaceControl, *item);
        auto rect = mUIProxy->rectCrop();
        if (rect) transaction->setBufferCrop(mSurfaceControl, *rect);
        transaction->apply();
    }
}

void BaseWindow::handleBufferReleased(int32_t bufKey) {
    std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
    if (buffProducer.get() == nullptr) {
        ALOGW("buffProducer is not existed!");
        return;
    }
    auto buffer = buffProducer->syncFreeState(bufKey);
    if (!buffer) {
        ALOGE("bufferReleased, release %d failure!", bufKey);
    }
}

void BaseWindow::updateOrCreateBufferQueue() {
    if (mSurfaceControl->bufferQueue() != nullptr) {
        mSurfaceControl->bufferQueue()->update(mSurfaceControl);
    } else {
        std::shared_ptr<BufferProducer> buffProducer =
                std::make_shared<BufferProducer>(mSurfaceControl);
        mSurfaceControl->setBufferQueue(buffProducer);
    }
}

void BaseWindow::setMockUIEventCallback(const MOCKUI_EVENT_CALLBACK& cb) {
    if (mUIProxy.get() == nullptr) return;
    mUIProxy->setEventCallback(cb);
}

} // namespace wm
} // namespace os
