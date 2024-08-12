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
#include <nuttx/tls.h>
#include <utils/RefBase.h>

#include "../common/WindowUtils.h"
#include "DummyDriverProxy.h"
#include "LVGLDriverProxy.h"
#include "SurfaceTransaction.h"
#include "uv.h"

static void _wm_timer_cb(uv_timer_t* handle) {
    uint32_t sleep_ms = ::os::wm::LVGLDriverProxy::timerHandler();

    if (sleep_ms == UI_PROXY_TIMER_READY) {
        FLOGD("stop timer event.");
        uv_timer_stop(handle);
        return;
    }

    /* Prevent busy loops. */

    if (sleep_ms == 0) {
        sleep_ms = 1;
    }

    FLOGV("sleep_ms = %" PRIu32, sleep_ms);
    uv_timer_start(handle, _wm_timer_cb, sleep_ms, 0);
}

static void _wm_timer_resume(void* data) {
    uv_timer_t* timer = (uv_timer_t*)data;
    FLOGD("resume timer event.");
    if (timer) uv_timer_start(timer, _wm_timer_cb, 0, 0);
}

namespace os {
namespace wm {

static inline bool getWindowService(sp<IWindowManager>& service) {
    if (service == nullptr || !android::IInterface::asBinder(service)->isBinderAlive()) {
        if (android::getService<IWindowManager>(android::String16(WindowManager::name()),
                                                &service) != android::NO_ERROR) {
            FLOGE("ServiceManager can't find the service:%s", WindowManager::name());
            return false;
        }
        return true;
    }
    return true;
}

std::shared_ptr<InputMonitor> WindowManager::monitorInput(const ::std::string& name,
                                                          int32_t displayId) {
    sp<IWindowManager> service;
    if (!getWindowService(service)) {
        FLOGE("get service failure!");
        return nullptr;
    }

    InputChannel* channel = new InputChannel();
    sp<IBinder> token = new BBinder();
    Status status = service->monitorInput(token, name, displayId, channel);
    if (!status.isOk()) {
        FLOGE("failure");
        return nullptr;
    }
    FLOGI("success");
    return std::make_shared<InputMonitor>(token, channel);
}

void WindowManager::releaseInput(InputMonitor* monitor) {
    if (!monitor) return;

    sp<IWindowManager> service;
    if (!getWindowService(service)) {
        FLOGE("get service failure!");
        return;
    }
    auto token = monitor->getToken();
    if (!token) return;

    Status status = service->releaseInput(token);
    if (!status.isOk()) {
        FLOGI("failure");
        return;
    }
    FLOGI("success");
}

WindowManager::WindowManager() : mService(nullptr), mTimerInited(false) {
    mTransaction = std::make_shared<SurfaceTransaction>();
    mTransaction->setWindowManager(this);
    getService();

    // init display size
    DisplayInfo displayInfo;
    int32_t result = 0;
    mService->getPhysicalDisplayInfo(1, &displayInfo, &result);
    mDispWidth = displayInfo.width;
    mDispHeight = displayInfo.height;
    LVGLDriverProxy::init();
}

WindowManager::~WindowManager() {
    toBackground();
    mWindows.clear();
    mService = nullptr;
    LVGLDriverProxy::deinit();
    FLOGD("WindowManager destructor");
}

sp<IWindowManager>& WindowManager::getService() {
    std::lock_guard<std::mutex> scoped_lock(mLock);
    getWindowService(mService);
    return mService;
}

std::shared_ptr<BaseWindow> WindowManager::newWindow(::os::app::Context* context) {
    WM_PROFILER_BEGIN();
    std::shared_ptr<BaseWindow> window = std::make_shared<BaseWindow>(context, this);
    FLOGI("%p", window.get());
    mWindows.push_back(window);

    // for lvgl driver
    auto proxy = std::make_shared<::os::wm::LVGLDriverProxy>(window);
    window->setUIProxy(std::dynamic_pointer_cast<::os::wm::UIDriverProxy>(proxy));
    if (!mTimerInited) {
        uv_timer_init(context->getMainLoop()->get(), &mEventTimer);
        uv_timer_start(&mEventTimer, _wm_timer_cb, proxy->getTimerPeriod(), 0);
        LVGLDriverProxy::setTimerResumeHandler(_wm_timer_resume, &mEventTimer);
        FLOGD("init event timer.");
        /* for video feature */
        vg_uv_init(context->getMainLoop()->get());
        mTimerInited = true;
    }

    WM_PROFILER_END();

    return window;
}

int32_t WindowManager::attachIWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    FLOGI("%p", window.get());

    sp<IWindow> w = window->getIWindow();
    LayoutParams lp = window->getLayoutParams();
    int32_t result = 0;

    InputChannel* outInputChannel = nullptr;
    if (lp.hasInput()) outInputChannel = new InputChannel();

    Status status = mService->addWindow(w, lp, LayoutParams::WINDOW_VISIBLE, 0, 1, outInputChannel,
                                        &result);
    if (status.isOk()) {
        window->setInputChannel(outInputChannel);
    } else {
        if (outInputChannel) delete outInputChannel;
        result = -1;
    }
    WM_PROFILER_END();

    return result;
}

void WindowManager::relayoutWindow(std::shared_ptr<BaseWindow> window) {
    WM_PROFILER_BEGIN();
    LayoutParams lp = window->getLayoutParams();
    FLOGI("%p, pos(%" PRId32 "x%" PRId32 "), size(%" PRId32 "x%" PRId32 ")", window.get(), lp.mX,
          lp.mY, lp.mWidth, lp.mHeight);
    sp<IBinder> handle = sp<BBinder>::make();
    SurfaceControl* surfaceControl =
            new SurfaceControl(lp.mToken, handle, lp.mWidth, lp.mHeight, lp.mFormat);
    int32_t result = 0;
    Status status = mService->relayout(window->getIWindow(), lp, lp.mWidth, lp.mHeight,
                                       window->getVisibility(), surfaceControl, &result);

    if (!status.isOk()) {
        FLOGE("relayout window failure!");
        window->setSurfaceControl(nullptr);

        /* clean local resource */
        handle.clear();
        delete surfaceControl;
    } else {
        window->setSurfaceControl(surfaceControl);
    }

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
    if (mWindows.size() == 0) {
        if (mTimerInited) {
            LVGLDriverProxy::setTimerResumeHandler(NULL, NULL);
            uv_close((uv_handle_t*)&mEventTimer, NULL);
            mTimerInited = false;
            FLOGD("close event timer.");
            vg_uv_deinit();
        }
    }
    WM_PROFILER_END();
    FLOGD("done");

    return 0;
}

void WindowManager::toBackground() {}

bool WindowManager::dumpWindows() {
    int number = 0;
    for (const auto& ptr : mWindows) {
        LayoutParams lp = ptr->getLayoutParams();
        FLOGI("Window %d", ++number);
        FLOGI("\t\t size:%" PRId32 "x%" PRId32 "", lp.mWidth, lp.mHeight);
        FLOGI("\t\t position:[%" PRId32 ",%" PRId32 "]", lp.mX, lp.mY);
        FLOGI("\t\t visibility:%" PRId32 "", ptr->getVisibility());
        FLOGI("\t\t type:%" PRId32 "", lp.mType);
        FLOGI("\t\t flags:%" PRId32 "", lp.mFlags);
        FLOGI("\t\t format:%" PRId32 "", lp.mFormat);
    }
    return true;
}

} // namespace wm
} // namespace os
