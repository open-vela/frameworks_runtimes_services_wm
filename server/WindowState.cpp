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

#define LOG_TAG "WindowState"

#include "WindowState.h"

#include <mqueue.h>
#include <sys/mman.h>
#include <utils/RefBase.h>

#include "RootContainer.h"
#include "WindowManagerService.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

WindowState::WindowState(WindowManagerService* service, const sp<IWindow>& window,
                         WindowToken* token, const LayoutParams& params, int32_t visibility)
      : mClient(window), mToken(token), mService(service), mInputChannel(nullptr) {
    mAttrs = params;
    mVisibility = visibility != 0 ? true : false;

    Rect rect(params.mX, params.mY, params.mX + params.mWidth, params.mY + params.mHeight);
    // TODO: config layer by type
    mNode = new WindowNode(this, mService->getRootContainer()->getDefLayer(), rect);
}

WindowState::~WindowState() {
    // TODO: clear members

    delete mNode;
}

std::shared_ptr<BufferConsumer> WindowState::getBufferConsumer() {
    if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferConsumer>(mSurfaceControl->bufferQueue());
    }
    return nullptr;
}

std::shared_ptr<InputChannel> WindowState::createInputChannel(const std::string name) {
    if (mInputChannel != nullptr) {
        ALOGE("mInputChannel is existed,create failed");
        return nullptr;
    }
    struct mq_attr mqstat;
    int oflag = O_CREAT | O_RDWR | O_NONBLOCK;
    mqd_t messageQueue = 0;

    memset(&mqstat, 0, sizeof(mqstat));
    mqstat.mq_maxmsg = MAX_MSG;
    mqstat.mq_msgsize = sizeof(InputMessage);
    mqstat.mq_flags = 0;

    if (((mqd_t)-1) == (messageQueue = mq_open(name.c_str(), oflag, 0777, &mqstat))) {
        ALOGI("mq_open doesn't return success ");
        return nullptr;
    }

    mInputChannel = std::make_shared<InputChannel>();
    mInputChannel->setEventFd(messageQueue);
    return mInputChannel;
}

void WindowState::setViewVisibility(bool visibility) {
    mVisibility = visibility;
    // TODO: NOTIFY WIN NODE
}

void WindowState::sendAppVisibilityToClients() {
    mVisibility = mToken->isClientVisible();
    mClient->dispatchAppVisibility(mVisibility);
}

std::shared_ptr<SurfaceControl> WindowState::createSurfaceControl(vector<BufferId> ids) {
    setHasSurface(false);

    sp<IBinder> handle = new BBinder();
    mSurfaceControl = std::make_shared<SurfaceControl>(mAttrs.mToken, handle, mAttrs.mWidth,
                                                       mAttrs.mHeight, mAttrs.mFormat);

    mSurfaceControl->initBufferIds(ids);
    std::shared_ptr<BufferConsumer> buffConsumer =
            std::make_shared<BufferConsumer>(mSurfaceControl);
    mSurfaceControl->setBufferQueue(buffConsumer);

    setHasSurface(true);

    return mSurfaceControl;
}

void WindowState::applyTransaction(LayerState layerState) {
    ALOGI("applyTransaction");
    // 1.acquireBuffer
    std::shared_ptr<BufferConsumer> buffQueue = getBufferConsumer();

    buffQueue->syncQueuedState(layerState.mBufferKey);

    BufferItem* buffItem = buffQueue->acquireBuffer();
    // 2.draw Buffer

    // 3.releaseBuffer
    buffQueue->releaseBuffer(buffItem);
    // 4.notify client buffer released

    // release befored buffer
    mClient->bufferReleased(layerState.mBufferKey);
}

bool WindowState::scheduleVsync(VsyncRequest vsyncReq) {
    // TODO: scheduleVsync
    return true;
}

void WindowState::removeIfPossible() {
    // mSurfaceController.hide

    // destroySurfaceLocked
    // windowRemovedLocked
    // win node
    // unlinkToDeath

    ALOGI("called");
}

BufferItem* WindowState::acquireBuffer() {
    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer == nullptr) {
        return nullptr;
    }
    return consumer->acquireBuffer();
}

bool WindowState::releaseBuffer(BufferItem* buffer) {
    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer && consumer->releaseBuffer(buffer) && mClient) {
        mClient->bufferReleased(buffer->mKey);
        return true;
    }

    return false;
}

} // namespace wm
} // namespace os
