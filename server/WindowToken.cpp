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

#define LOG_TAG "WindowToken"

#include "WindowToken.h"

#include "WindowState.h"
#include "WindowUtils.h"

namespace os {
namespace wm {

WindowToken::WindowToken(WindowManagerService* service, const sp<IBinder>& token, int32_t type,
                         int32_t displayId, int32_t clientPid)
      : mService(service),
        mToken(token),
        mType(type),
        mClientVisible(false),
        mClientVisibility(LayoutParams::WINDOW_GONE),
        mClientPid(clientPid) {}

WindowToken::~WindowToken() {
    mToken = nullptr;
    mChildren.clear();
    mClientPid = -1;
}

void WindowToken::addWindow(WindowState* win) {
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        if ((*it) == win) {
            FLOGW("[%d] window has been attached.\n", mClientPid);
            return;
        }
    }
    mChildren.push_back(win);
    FLOGI("[%d] add window ok, now child count:%d", mClientPid, mChildren.size());
}

bool WindowToken::isClientVisible() {
    return mClientVisible;
}

void WindowToken::setClientVisibility(int32_t visibility) {
    if (mClientVisibility == visibility) return;
    FLOGI("[%d] %d => %d (0:visible, 1:hold, 2:gone)", mClientPid, mClientVisibility, visibility);

    mClientVisibility = visibility;
    if (visibility == LayoutParams::WINDOW_HOLD)
        return;

    bool visible = visibility == LayoutParams::WINDOW_VISIBLE ? true : false;
    setClientVisible(visible);
}

void WindowToken::setClientVisible(bool clientVisible) {
    if (mClientVisible == clientVisible) {
        return;
    }
    FLOGW("[%d] %s => %s", mClientPid, mClientVisible ? "visible" : "invisible",
          clientVisible ? "visible" : "invisible");
    mClientVisible = clientVisible;
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        (*it)->sendAppVisibilityToClients();
    }
}

void WindowToken::removeAllWindowsIfPossible() {
    FLOGI("[%d]", mClientPid);
    for (auto winState : mChildren) {
        winState->removeIfPossible();
        delete winState;
        winState = nullptr;
    }
}

} // namespace wm
} // namespace os
