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

#define LOG_TAG "WMS:Token"

#include "WindowToken.h"

#include <pm/PackageManager.h>

#include "../common/WindowUtils.h"
#include "WindowState.h"

namespace os {
namespace wm {

WindowToken::WindowToken(WindowManagerService* service, const sp<IBinder>& token, int32_t type,
                         int32_t displayId, int32_t clientPid, const std::string& packageName)
      : mService(service),
        mToken(token),
        mType(type),
        mClientVisibility(LayoutParams::WINDOW_GONE),
        mClientPid(clientPid),
        mPersistOnEmpty(false),
        mRemoved(false) {
    ::os::pm::PackageManager pm;
    if (pm.getPackageInfo(packageName, &mPackageInfo) != 0) {
        ALOGE("Unable to look up package:%s information", packageName.c_str());
    }
}

WindowToken::~WindowToken() {
    FLOGI("done");
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
    FLOGI("[%d] add window ok, now child count:%d", mClientPid, (int)mChildren.size());
}

void WindowToken::removeWindow(WindowState* win) {
    if (!win) return;

    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        if ((*it) == win) {
            mChildren.erase(it);
            break;
        }
    }
}

void WindowToken::setClientVisibility(int32_t visibility) {
    if (mClientVisibility == visibility) return;
    FLOGI("[%d] %s -> %s", mClientPid, LayoutParams::visibilityToName(mClientVisibility),
          LayoutParams::visibilityToName(visibility));

    mClientVisibility = visibility;
    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        (*it)->sendAppVisibilityToClients(mClientVisibility);
    }
}

void WindowToken::removeIfPossible() {
    if (mRemoved) return;
    mRemoved = true;

    for (auto it = mChildren.begin(); it != mChildren.end(); it++) {
        WindowState* state = *it;
        state->removeIfPossible();
    }

    mService->removeWindowTokenInner(mToken);
}

} // namespace wm
} // namespace os
