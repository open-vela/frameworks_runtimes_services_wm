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

#include "UIDriverProxy.h"
#include "wm/InputChannel.h"
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
    // TODO
    return Status::ok();
}

Status BaseWindow::W::bufferReleased(int32_t bufKey) {
    // TODO
    return Status::ok();
}

BaseWindow::BaseWindow() {}

BaseWindow::~BaseWindow() {}

BaseWindow::BaseWindow(::os::app::Context* context) : mContext(context) {}

bool BaseWindow::scheduleVsync(VsyncRequest freq) {
    // TODO:
    return false;
}

void* BaseWindow::getRoot() {
    return mUIProxy->getRoot();
}

void BaseWindow::dispatchAppVisibility(bool visible) {
    mContext->getMainLoop()->postTask(
            [this, visible](void*) { this->handleAppVisibility(visible); });
}

void BaseWindow::handleAppVisibility(bool visible) {
    if (mAppVisible == visible) {
        return;
    }
    mAppVisible = visible;
    relayoutWindow(mAttrs, mAppVisible);
}

int32_t BaseWindow::relayoutWindow(LayoutParams& params, int32_t windowVisibility) {
    int32_t result = -1;

    if (mSurfaceControl == nullptr) {
        sp<IBinder> handle = new BBinder();
        mSurfaceControl = std::make_shared<SurfaceControl>(params.mToken, handle, params.mWidth,
                                                           params.mHeight, params.mFormat);
    }

    mWindowManager->getService()->relayout(mIWindow, params, params.mWidth, params.mHeight,
                                           windowVisibility, mSurfaceControl.get(), &result);

    if (mSurfaceControl->isValid()) {
        updateOrCreateBufferQueue();
    } else {
        // TODO release mSurfaceControl
    }

    return result;
} // namespace wm

void BaseWindow::updateOrCreateBufferQueue() {
    if (mSurfaceControl->bufferQueue() != nullptr) {
        // TODO: updateBuffer
    } else {
        std::shared_ptr<BufferProducer> buffProducer =
                std::make_shared<BufferProducer>(mSurfaceControl);
        mSurfaceControl->setBufferQueue(buffProducer);
    }
}

} // namespace wm
} // namespace os
