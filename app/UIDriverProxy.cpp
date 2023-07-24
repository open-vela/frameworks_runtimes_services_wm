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

#include "UIDriverProxy.h"

#include "BaseWindow.h"
#include "os/wm/VsyncRequest.h"

namespace os {
namespace wm {

UIDriverProxy::UIDriverProxy(std::shared_ptr<BaseWindow> win)
      : mBaseWindow(win), mBufferItem(nullptr), mFlags(0) {}

UIDriverProxy::~UIDriverProxy() {}

void UIDriverProxy::onInvalidate(bool periodic) {
    if (!mBaseWindow.expired())
        mBaseWindow.lock()->scheduleVsync(periodic ? VsyncRequest::VSYNC_REQ_PERIODIC
                                                   : VsyncRequest::VSYNC_REQ_SINGLE);
}

void UIDriverProxy::onRectCrop(Rect& rect) {
    mFlags |= UIP_BUFFER_RECT_UPDATE;
    mRectCrop = rect;
}

Rect* UIDriverProxy::rectCrop() {
    return ((mFlags & UIP_BUFFER_RECT_UPDATE) == UIP_BUFFER_RECT_UPDATE) ? &mRectCrop : nullptr;
}

void* UIDriverProxy::onDequeueBuffer() {
    return (mBufferItem->mState == BSTATE_DEQUEUED) ? mBufferItem->mBuffer : nullptr;
}

bool UIDriverProxy::onQueueBuffer() {
    mFlags |= UIP_BUFFER_UPDATE;
    return true;
}

void UIDriverProxy::onCancelBuffer() {
    mFlags = 0;
}

void UIDriverProxy::drawFrame(BufferItem* bufItem) {
    mFlags = 0;
    mBufferItem = bufItem;
}

bool UIDriverProxy::finishDrawing() {
    return mFlags != 0 ? true : false;
}

void UIDriverProxy::setDrawCallback(const CUSTOM_DRAW_CALLBACK& cb) {}
} // namespace wm
} // namespace os
