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

#include "../system_server/BaseProfiler.h"
#include "DummyDriverProxy.h"
#include "LVGLDriverProxy.h"
#include "LogUtils.h"
#include "SurfaceTransaction.h"

namespace os {
namespace wm {

pthread_key_t WindowManager::mTLSKey;

WindowManager::WindowManager() {
    mTransaction = std::make_shared<SurfaceTransaction>();
    mTransaction->setWindowManager(this);
    getService();
}
WindowManager::~WindowManager() {
    mService = nullptr;
    mWindows.clear();
}

void WindowManager::destroy() {
    (void)pthread_setspecific(mTLSKey, NULL);
}

void WindowManager::destructTLSData(void* data) {
    if (data) {
        WindowManager* wm = (WindowManager*)data;
        delete wm;
    }
}

void WindowManager::createTLSKey() {
    (void)pthread_key_create(&mTLSKey, destructTLSData);
}

WindowManager* WindowManager::getInstance() {
    static pthread_once_t key_once = PTHREAD_ONCE_INIT;
    (void)pthread_once(&key_once, createTLSKey);

    WindowManager* instance;
    if ((instance = (WindowManager*)pthread_getspecific(mTLSKey)) == NULL) {
        instance = new WindowManager();
        if (instance) (void)pthread_setspecific(mTLSKey, instance);
    }

    return instance;
}

sp<IWindowManager>& WindowManager::getService() {
    std::lock_guard<std::mutex> scoped_lock(mLock);
    if (mService == nullptr || !android::IInterface::asBinder(mService)->isBinderAlive()) {
        if (android::getService<IWindowManager>(android::String16(WindowManager::name()),
                                                &mService) != android::NO_ERROR) {
            FLOGE("ServiceManager can't find the service:%s", WindowManager::name());
        }
    }
    return mService;
}

std::shared_ptr<BaseWindow> WindowManager::newWindow(::os::app::Context* context) {
    WM_PROFILER_BEGIN();
    std::shared_ptr<BaseWindow> window = std::make_shared<BaseWindow>(context);
    FLOGI("%p", window.get());
    window->setWindowManager(this);
    mWindows.push_back(window);

#ifndef CONFIG_WM_USE_LVGL
    // for dummy driver
    auto proxy = std::make_shared<::os::wm::DummyDriverProxy>(window);
    window->setUIProxy(std::dynamic_pointer_cast<::os::wm::UIDriverProxy>(proxy));
#else
    // for lvgl driver
    auto proxy = std::make_shared<::os::wm::LVGLDriverProxy>(window);
    window->setUIProxy(std::dynamic_pointer_cast<::os::wm::UIDriverProxy>(proxy));
#endif
    WM_PROFILER_END();

    return window;
}

int32_t WindowManager::attachIWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());
    sp<IWindow> w = window->getIWindow();
    int32_t result = 0;
    // TODO: create inputchannel if app need input event
    InputChannel* outInputChannel = new InputChannel();
    LayoutParams lp = window->getLayoutParams();
    DisplayInfo dispalyInfo;
    mService->getPhysicalDisplayInfo(1, &dispalyInfo, &result);
    if (lp.mWidth == lp.MATCH_PARENT) {
        lp.mWidth = dispalyInfo.width;
    }
    if (lp.mHeight == lp.MATCH_PARENT) {
        lp.mHeight = dispalyInfo.height;
    }
    window->setLayoutParams(lp);

    mService->addWindow(w, lp, 1, 0, 1, outInputChannel, &result);
    window->setInputChannel(outInputChannel);
    WM_PROFILER_END();

    return result;
}

void WindowManager::relayoutWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());
    LayoutParams params = window->getLayoutParams();
    sp<IBinder> handle = new BBinder();
    SurfaceControl* surfaceControl = new SurfaceControl(params.mToken, handle, params.mWidth,
                                                        params.mHeight, params.mFormat);
    int32_t result = 0;
    mService->relayout(window->getIWindow(), params, params.mWidth, params.mHeight,
                       window->getAppVisible(), surfaceControl, &result);
    window->setSurfaceControl(surfaceControl);
    WM_PROFILER_END();
}

bool WindowManager::removeWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());
    mService->removeWindow(window->getIWindow());
    window->doDie();
    auto it = std::find(mWindows.begin(), mWindows.end(), window);
    if (it != mWindows.end()) {
        mWindows.erase(it);
    }
    WM_PROFILER_END();
    FLOGD("done");

    return 0;
}

} // namespace wm
} // namespace os
