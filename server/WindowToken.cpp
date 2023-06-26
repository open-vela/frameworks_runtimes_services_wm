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

namespace os {
namespace wm {

WindowToken::WindowToken(WindowManagerService* service, const sp<IBinder>& token, int32_t type,
                         int32_t displayId)
      : mService(service), mToken(token), mType(type) {}

WindowToken::~WindowToken() {}

void WindowToken::addWindow(WindowState* win) {
    std::vector<WindowState*>::iterator it;
    for (it = mChildren.begin(); it != mChildren.end(); it++) {
        if ((*it) == win) {
            ALOGW("already attach in the mChildren\n");
            return;
        }
    }
    mChildren.push_back(win);
}

} // namespace wm
} // namespace os