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
WindowManager::WindowManager() {
    mTransaction = std::make_shared<SurfaceTransaction>();
    mTransaction->setWindowManager(this);
    getService();
}
WindowManager::~WindowManager() {
    mService = nullptr;
    mWindows.clear();
}

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
    mWindows.push_back(window);
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
    LayoutParams params = window->getLayoutParams();
    sp<IBinder> handle = new BBinder();
    SurfaceControl* surfaceControl = new SurfaceControl(params.mToken, handle, params.mWidth,
                                                        params.mHeight, params.mFormat);
    int32_t result = 0;
    mService->relayout(window->getIWindow(), params, params.mWidth, params.mHeight,
                       window->getAppVisible(), surfaceControl, &result);
    window->setSurfaceControl(surfaceControl);
}

bool WindowManager::removeWindow(std::shared_ptr<BaseWindow> window) {
    mService->removeWindow(window->getIWindow());
    window->doDie();
    auto it = std::find(mWindows.begin(), mWindows.end(), window);
    if (it != mWindows.end()) {
        mWindows.erase(it);
    }

    return 0;
}

} // namespace wm
} // namespace os