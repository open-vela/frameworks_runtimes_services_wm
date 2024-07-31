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

#include "../common/WindowUtils.h"
#include "BaseWindow.h"
#include "os/wm/VsyncRequest.h"

namespace os {
namespace wm {

UIDriverProxy::UIDriverProxy(std::shared_ptr<BaseWindow> win)
      : mBaseWindow(win),
        mBufferItem(nullptr),
        mFlags(0),
        mInputMonitor(nullptr),
        mEventListener(nullptr),
        mVsyncEnabled(false) {}

UIDriverProxy::~UIDriverProxy() {
    mBufferItem = nullptr;
    mEventListener = nullptr;
    mInputMonitor = nullptr;
}

bool UIDriverProxy::onInvalidate(bool periodic) {
    if (!mBaseWindow.expired()) {
        return mBaseWindow.lock()->scheduleVsync(periodic ? VsyncRequest::VSYNC_REQ_PERIODIC
                                                          : VsyncRequest::VSYNC_REQ_SINGLESUPPRESS);
    }
    return false;
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

void UIDriverProxy::setInputMonitor(InputMonitor* monitor) {
    mInputMonitor = monitor;
}

bool UIDriverProxy::readEvent(InputMessage* message) {
    if (message && mInputMonitor) {
        return mInputMonitor->receiveMessage(message);
    }
    return false;
}

void UIDriverProxy::updateResolution(int32_t width, int32_t height, uint32_t format) {}

void UIDriverProxy::updateVisibility(bool visible) {}

void UIDriverProxy::onFBVsyncRequest(bool enable) {
    FLOGI("listen vsync event from %s to %s", mVsyncEnabled ? "enable" : "disable",
          enable ? "enable" : "disable");
    if (mVsyncEnabled == enable) return;

    mVsyncEnabled = enable;
    /* update vsync reqeust to server */
    onInvalidate(mVsyncEnabled);
}

} // namespace wm
} // namespace os
