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

#include "../common/WindowUtils.h"
#include "WindowState.h"

namespace os {
namespace wm {

WindowToken::WindowToken(WindowManagerService* service, const sp<IBinder>& token, int32_t type,
                         int32_t displayId, int32_t clientPid)
      : mService(service),
        mToken(token),
        mType(type),
        mClientVisibility(LayoutParams::WINDOW_GONE),
        mClientPid(clientPid),
        mPersistOnEmpty(false),
        mRemoved(false) {}

WindowToken::~WindowToken() {
    FLOGI("");
    mChildren.clear();
    mClientPid = -1;
}

void WindowToken::addWindow(WindowState* win) {
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        if ((*it) == win) {
            FLOGW("[%" PRId32 "] window has been attached.\n", mClientPid);
            return;
        }
    }
    mChildren.push_back(win);
    FLOGI("[%" PRId32 "] add window ok, now child count:%d", mClientPid, mChildren.size());
}

void WindowToken::removeWindow(WindowState* win) {
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        if ((*it) == win) {
            mChildren.erase(it);
            return;
        }
    }
}

void WindowToken::setClientVisibility(int32_t visibility) {
    if (mClientVisibility == visibility) return;
    FLOGI("[%" PRId32 "] %" PRId32 " => %" PRId32 " (0:visible, 1:hold, 2:gone)", mClientPid,
          mClientVisibility, visibility);

    mClientVisibility = visibility;
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        (*it)->sendAppVisibilityToClients(mClientVisibility);
    }
}

void WindowToken::removeIfPossible() {
    if (mRemoved) return;
    mRemoved = true;

    for (auto it = mChildren.begin(); it != mChildren.end();) {
        WindowState* state = *it;
        it = mChildren.erase(it);
        state->removeIfPossible();
    }

    mService->removeWindowTokenInner(mToken);
}

} // namespace wm
} // namespace os
