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

#include "UIDriverProxy.h"
#include "uv.h"
#include "wm/InputChannel.h"
#include "wm/InputMessage.h"
#include "wm/SurfaceControl.h"

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
        InputMessage message;
        mq_receive(fd, (char*)&message, sizeof(InputMessage), NULL);
        // TODO: send event to lvgl
    }
}

BaseWindow::BaseWindow() {}

BaseWindow::~BaseWindow() {}

BaseWindow::BaseWindow(::os::app::Context* context) : mContext(context) {
    mIWindow = sp<W>::make(this);
    // TODO init lvgl instance
}

void BaseWindow::setWindowManager(WindowManager* wm) {
    mWindowManager = wm;
}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
    mWindowManager->getService()->requestVsync(getIWindow(), freq);
    return true;
}

void* BaseWindow::getRoot() {
    return mUIProxy->getRoot();
}

std::shared_ptr<BufferProducer> BaseWindow::getBufferProducer() {
    if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferProducer>(mSurfaceControl->bufferQueue());
    }
    return nullptr;
}

void BaseWindow::doDie() {
    // TODO destory lvglDriver
    mInputChannel = nullptr;
    mSurfaceControl = nullptr;
    delete mPoll;
}

void BaseWindow::setInputChannel(InputChannel* inputChannel) {
    if (inputChannel != nullptr && inputChannel->isValid()) {
        mInputChannel.reset(inputChannel);
        mPoll = new ::os::app::UvPoll(mContext->getMainLoop(), mInputChannel->getEventFd());
        mPoll->start(UV_READABLE, eventCallback, this);
    }
}

void BaseWindow::dispatchAppVisibility(bool visible) {
    mContext->getMainLoop()->postTask(
            [this, visible](void*) { this->handleAppVisibility(visible); });
}

void BaseWindow::onFrame(int32_t seq) {
    mContext->getMainLoop()->postTask([this, seq](void*) { this->handleOnFrame(seq); });
}

void BaseWindow::bufferReleased(int32_t bufKey) {
    mContext->getMainLoop()->postTask([this, bufKey](void*) { this->handleBufferRelease(bufKey); });
}

void BaseWindow::handleAppVisibility(bool visible) {
    if (mAppVisible == visible) {
        return;
    }
    mAppVisible = visible;
    mWindowManager->relayoutWindow(shared_from_this());
    if (mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        // release mSurfaceControl
        mSurfaceControl = nullptr;
    }
}

void BaseWindow::handleOnFrame(int32_t seq) {
    if (!mSurfaceControl->isValid()) {
        mWindowManager->relayoutWindow(shared_from_this());
    } else {
        // 1. dequeue a buffer
        std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();

        BufferItem* buffItem = buffProducer->dequeueBuffer();

        // TODO:2.lv display draw fill buffer

        // 3. queue the buffer to display
        buffProducer->queueBuffer(buffItem);

        mWindowManager->getTransaction()->setBuffer(mSurfaceControl, *buffItem).apply();
    }
}

void BaseWindow::handleBufferRelease(int32_t bufKey) {
    std::shared_ptr<BufferProducer> buffProducer = getBufferProducer();
    buffProducer->syncFreeState(bufKey);
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

} // namespace wm
} // namespace os
