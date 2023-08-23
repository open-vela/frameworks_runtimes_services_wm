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

#define LOG_TAG "RootContainer"

#include "RootContainer.h"

#include "wm/UIInstance.h"

#if LVGL_VERSION_MAJOR < 9
#include <lv_porting/lv_porting.h>
#endif

#include <lvgl/lvgl.h>
#include <utils/Log.h>

namespace os {
namespace wm {

RootContainer::RootContainer() : mDisp(nullptr), mSyncMode(DSM_TIMER) {
#ifdef CONFIG_FB_SYNC
    mVsyncEvent = 0;
#else
    mVsyncEvent = -1;
#endif
    init();
}

RootContainer::~RootContainer() {
    mDisp = nullptr;
    UIDeinit();
}

lv_disp_t* RootContainer::getRoot() {
    return mDisp;
}

lv_obj_t* RootContainer::getDefLayer() {
    return lv_disp_get_scr_act(mDisp);
}

lv_obj_t* RootContainer::getSysLayer() {
    return lv_disp_get_layer_sys(mDisp);
}

lv_obj_t* RootContainer::getTopLayer() {
    return lv_disp_get_layer_top(mDisp);
}

bool RootContainer::init() {
    UIInit();

#if LVGL_VERSION_MAJOR >= 9
#ifdef CONFIG_LV_USE_LINUX_FBDEV
    lv_linux_fbdev_set_file(lv_linux_fbdev_create(), CONFIG_LV_FBDEV_INTERFACE_DEFAULT_DEVICEPATH);
#endif

#ifdef CONFIG_LV_USE_NUTTX_TOUCHSCREEN
    lv_nuttx_touchscreen_create(CONFIG_LV_TOUCHPAD_INTERFACE_DEFAULT_DEVICEPATH);
#endif

    mDisp = lv_disp_get_default();

    lv_timer_t* timer = _lv_disp_get_refr_timer(mDisp);
    if (timer) {
        lv_timer_set_period(timer, CONFIG_WINDOW_REFRESH_PERIOD);
    }

#else
    lv_porting_init();
    mDisp = lv_disp_get_default();
#endif

    return mDisp ? true : false;
}

bool RootContainer::drawFrame() {
    lv_timer_handler();
    return true;
}

bool RootContainer::handleEvent(int fd, int events) {
    if (events == mVsyncEvent) {
        return drawFrame();
    }
    return false;
}

int RootContainer::getFdInfo(int* fd, int* events) {
    if (mSyncMode == DSM_VSYNC) {
        if (fd) {
            *fd = -1; // TODO: update it
        }
        if (events) {
            *events = mVsyncEvent;
        }
        return 0;
    }
    return -1;
}

bool RootContainer::getDisplayInfo(DisplayInfo* info) {
    if (info) {
        info->width = lv_disp_get_hor_res(mDisp);
        info->height = lv_disp_get_ver_res(mDisp);
        return true;
    }

    return false;
}

} // namespace wm
} // namespace os
