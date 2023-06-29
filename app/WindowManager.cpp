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

#define LOG_TAG "WindowManager"

#include "WindowManager.h"

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <utils/RefBase.h>

namespace os {
namespace wm {
WindowManager::WindowManager() {}
WindowManager::~WindowManager() {}

sp<IWindowManager>& WindowManager::getService() {
    std::lock_guard<std::mutex> scoped_lock(mLock);
    if (mService == nullptr || !android::IInterface::asBinder(mService)->isBinderAlive()) {
        if (android::getService<IWindowManager>(android::String16(WindowManager::name()),
                                                &mService) != android::NO_ERROR) {
            ALOGE("ServiceManager can't find the service:%s", WindowManager::name());
        }
    }
    return mService;
}

std::shared_ptr<BaseWindow> WindowManager::newWindow(::os::app::Context* context) {
    std::shared_ptr<BaseWindow> window = std::make_shared<BaseWindow>(context);
    return window;
}

int32_t WindowManager::attachIWindow(std::shared_ptr<BaseWindow> window) {
    sp<IWindow> w = window->getIWindow();
    int32_t result = 0;
    // TODO: create inputchannel if app need input event
    InputChannel* outInputChannel = new InputChannel();
    mService->addWindow(w, window->getLayoutParams(), 1, 0, 1, outInputChannel, &result);
    window->setInputChannel(outInputChannel);

    return result;
}

void WindowManager::relayoutWindow(std::shared_ptr<BaseWindow> window) {
    // TODO
}

bool WindowManager::removeWindow(std::shared_ptr<BaseWindow> window) {
    // TODO
    return 0;
}

} // namespace wm
} // namespace os