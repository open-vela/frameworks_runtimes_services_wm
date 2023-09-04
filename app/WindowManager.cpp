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
#include "uv.h"

#if LV_VERSION_CHECK(9, 0, 0)
#include "wm/UIInstance.h"
#endif

#include <nuttx/tls.h>

static void _lv_timer_cb(uv_timer_t*) {
#if LV_VERSION_CHECK(9, 0, 0)
    lv_timer_handler();
#endif
}

namespace os {
namespace wm {

static void _wm_inst_free(FAR void* instance) {
    FLOGI("free wm instance for thread(%d)", pthread_self());
    if (instance) {
        WindowManager* wm = (WindowManager*)instance;
        delete wm;
    }
}

WindowManager::WindowManager() : mTimerInited(false) {
    mTransaction = std::make_shared<SurfaceTransaction>();
    mTransaction->setWindowManager(this);
    getService();

    // init display size
    DisplayInfo displayInfo;
    int32_t result = 0;
    mService->getPhysicalDisplayInfo(1, &displayInfo, &result);
    mDispWidth = displayInfo.width;
    mDispHeight = displayInfo.height;

#if LV_VERSION_CHECK(9, 0, 0)
    UIInit();
#endif
}

WindowManager::~WindowManager() {
    toBackground();
#if LV_VERSION_CHECK(9, 0, 0)
    UIDeinit();
#endif
    mService = nullptr;
    mWindows.clear();
}

WindowManager* WindowManager::getInstance() {
    static int index = -1;
    WindowManager* instance;

    if (index < 0) {
        index = task_tls_alloc(_wm_inst_free);
    }

    if (index >= 0) {
        instance = (WindowManager*)task_tls_get_value(index);
        if (instance == NULL) {
            instance = new WindowManager();
            task_tls_set_value(index, (uintptr_t)instance);
            FLOGI("new wm instance for thread(%d)", pthread_self());
        }
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

#if LV_VERSION_CHECK(9, 0, 0)
    // for lvgl driver
    auto proxy = std::make_shared<::os::wm::LVGLDriverProxy>(window);
    window->setUIProxy(std::dynamic_pointer_cast<::os::wm::UIDriverProxy>(proxy));
#else
    // for dummy driver
    auto proxy = std::make_shared<::os::wm::DummyDriverProxy>(window);
    window->setUIProxy(std::dynamic_pointer_cast<::os::wm::UIDriverProxy>(proxy));
#endif
    if (!mTimerInited) {
        uv_timer_init(context->getMainLoop()->get(), &mEventTimer);
        mTimerInited = true;
    }
    WM_PROFILER_END();

    return window;
}

int32_t WindowManager::attachIWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());

    sp<IWindow> w = window->getIWindow();
    int32_t result = 0;
    InputChannel* outInputChannel = nullptr;
    LayoutParams lp = window->getLayoutParams();
    DisplayInfo displayInfo;

    if (lp.mWidth == lp.MATCH_PARENT) {
        lp.mWidth = mDispWidth;
    }
    if (lp.mHeight == lp.MATCH_PARENT) {
        lp.mHeight = mDispHeight;
    }
    window->setLayoutParams(lp);

    if (lp.hasInput()) outInputChannel = new InputChannel();

    Status status = mService->addWindow(w, lp, 0, 0, 1, outInputChannel, &result);
    if (status.isOk()) {
        window->setInputChannel(outInputChannel);

#if LV_VERSION_CHECK(9, 0, 0)
        if (mTimerInited && mEventTimer.timer_cb == NULL) {
            uv_timer_start(&mEventTimer, _lv_timer_cb, CONFIG_LV_DEF_REFR_PERIOD,
                           CONFIG_LV_DEF_REFR_PERIOD);
        }
#endif
    } else {
        if (outInputChannel) delete outInputChannel;
        result = -1;
    }
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
                       window->isVisible(), surfaceControl, &result);
    window->setSurfaceControl(surfaceControl);
    WM_PROFILER_END();
}

bool WindowManager::removeWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());

    mTransaction->clean();
    mService->removeWindow(window->getIWindow());
    window->doDie();
    auto it = std::find(mWindows.begin(), mWindows.end(), window);
    if (it != mWindows.end()) {
        mWindows.erase(it);
    }
#if LV_VERSION_CHECK(9, 0, 0)
    if (mWindows.size() == 0) {
        toBackground();
    }
#endif
    WM_PROFILER_END();
    FLOGD("done");

    return 0;
}

void WindowManager::toBackground() {
    if (mTimerInited && mEventTimer.timer_cb) {
        uv_timer_stop(&mEventTimer);
        mEventTimer.timer_cb = NULL;
    }
}

bool WindowManager::dumpWindows() {
    int number = 1;
    for (const auto& ptr : mWindows) {
        LayoutParams attrs = ptr->getLayoutParams();
        bool appVsible = ptr->isVisible();
        FLOGI("Window %d", number);
        FLOGI("\t\t size:%dx%d", attrs.mWidth, attrs.mHeight);
        FLOGI("\t\t position:[%d,%d]", attrs.mX, attrs.mY);
        FLOGI("\t\t visible:%d", appVsible);
        FLOGI("\t\t type:%d", attrs.mType);
        FLOGI("\t\t flags:%d", attrs.mFlags);
        FLOGI("\t\t format:%d", attrs.mFormat);
        number++;
    }
    return true;
}

} // namespace wm
} // namespace os
