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

#define LOG_TAG "WindowManagerService"

#include "WindowManagerService.h"

#include <utils/Log.h>

#include "WindowState.h"
#include "WindowToken.h"

namespace os {
namespace wm {

WindowManagerService::WindowManagerService() {}
WindowManagerService::~WindowManagerService() {}

// implement AIDL interfaces
Status WindowManagerService::getPhysicalDisplayInfo(int32_t displayId, DisplayInfo* info,
                                                    int32_t* _aidl_return) {
    // TODO
    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::addWindow(const sp<IWindow>& window, const LayoutParams& attrs,
                                       int32_t visibility, int32_t displayId, int32_t userId,
                                       InputChannel* outInputChannel, int32_t* _aidl_return) {
    sp<IBinder> client = IInterface::asBinder(window);
    WindowStateMap::iterator itState = mWindowMap.find(client);
    if (itState != mWindowMap.end()) {
        ALOGI("window is already existed");
        *_aidl_return = -1; // ERROR
        return Status::ok();
    }

    sp<IBinder> token = attrs.mToken; // get token from attrs
    auto itToken = mTokenMap.find(token);
    if (itToken == mTokenMap.end()) {
        *_aidl_return = -1; // ERROR
        return Status::ok();
    }
    WindowToken* winToken = itToken->second;

    WindowState* win = new WindowState(this, window, winToken, attrs, visibility);
    mWindowMap.emplace(client, win);

    winToken->addWindow(win);

    if (outInputChannel != nullptr) {
        // TODO: need define unique name
        std::shared_ptr<InputChannel> inputChannel = win->createInputChannel("/data/InputMessage");
        outInputChannel->copyFrom(*inputChannel);
    }

    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::removeWindow(const sp<IWindow>& window) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::relayout(const sp<IWindow>& window, const LayoutParams& attrs,
                                      int32_t requestedWidth, int32_t requestedHeight,
                                      int32_t visibility, SurfaceControl* outSurfaceControl,
                                      int32_t* _aidl_return) {
    // TODO
    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::isWindowToken(const sp<IBinder>& binder, bool* _aidl_return) {
    // TODO
    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::addWindowToken(const sp<IBinder>& token, int32_t type,
                                            int32_t displayId) {
    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        ALOGW("windowToken is already existed");
        return Status::ok();
    } else {
        WindowToken* windToken = new WindowToken(this, token, type, displayId);
        mTokenMap.emplace(token, windToken);
    }
    return Status::ok();
}

Status WindowManagerService::removeWindowToken(const sp<IBinder>& token, int32_t displayId) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::updateWindowTokenVisibility(const sp<IBinder>& token,
                                                         int32_t visibility) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::applyTransaction(const vector<LayerState>& state) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::requestVsync(const sp<IWindow>& window, VsyncRequest freq) {
    // TODO
    return Status::ok();
}

} // namespace wm
} // namespace os