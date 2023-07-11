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

#include <sys/mman.h>
#include <utils/RefBase.h>

#include "RootContainer.h"
#include "WindowManagerService.h"
#include "wm/LayerState.h"

namespace os {
namespace wm {

WindowState::WindowState(WindowManagerService* service, const sp<IWindow>& window,
                         WindowToken* token, const LayoutParams& params, int32_t visibility)
      : mClient(window),
        mToken(token),
        mService(service),
        mInputChannel(nullptr),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mFrameReq(0),
        mHasSurface(false) {
    mAttrs = params;
    mVisibility = visibility != 0 ? true : false;

    Rect rect(params.mX, params.mY, params.mX + params.mWidth, params.mY + params.mHeight);
    // TODO: config layer by type
    mNode = new WindowNode(this, mService->getRootContainer()->getDefLayer(), rect);
}

WindowState::~WindowState() {
    // TODO: clear members
    mClient = nullptr;
    mToken = nullptr;
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
    mInputChannel = std::make_shared<InputChannel>();
    if (mInputChannel->create(name)) {
        return mInputChannel;
    } else {
        return nullptr;
    }
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

void WindowState::destorySurfaceControl() {
    mSurfaceControl.reset();
}

void WindowState::destoryInputChannel() {
    mInputChannel->release();
    mInputChannel.reset();
}

void WindowState::applyTransaction(LayerState layerState) {
    BufferItem* buffItem = nullptr;
    Rect* rect = nullptr;
    if (layerState.mFlags & LayerState::LAYER_POSITION_CHANGED) {
        // TODO
    }

    if (layerState.mFlags & LayerState::LAYER_ALPHA_CHANGED) {
        // TODO
    }

    if (layerState.mFlags & LayerState::LAYER_BUFFER_CHANGED) {
        std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
        buffItem = consumer->syncQueuedState(layerState.mBufferKey);
    }

    if (layerState.mFlags & LayerState::LAYER_BUFFER_CROP_CHANGED) {
        rect = &layerState.mBufferCrop;
    }

    mNode->updateBuffer(buffItem, rect);
}

bool WindowState::scheduleVsync(VsyncRequest vsyncReq) {
    if (mVsyncRequest == vsyncReq) return false;

    mVsyncRequest = vsyncReq;
    return true;
}

bool WindowState::onVsync() {
    switch (mVsyncRequest) {
        case VsyncRequest::VSYNC_REQ_NONE:
            return false;

        case VsyncRequest::VSYNC_REQ_SINGLE:
            mVsyncRequest = VsyncRequest::VSYNC_REQ_NONE;
            break;

        case VsyncRequest::VSYNC_REQ_SINGLESUPPRESS:
            mVsyncRequest = VsyncRequest::VSYNC_REQ_SINGLE;
            break;

        case VsyncRequest::VSYNC_REQ_PERIODIC:
            break;

        default:
            return false;
    }

    mClient->onFrame(++mFrameReq);
    return true;
}

void WindowState::removeIfPossible() {
    destorySurfaceControl();
    destoryInputChannel();
    mToken.reset();
    // win node
    mNode = nullptr;

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
