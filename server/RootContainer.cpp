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

#include <lv_porting/lv_porting.h>
#include <lvgl/lvgl.h>
#include <nuttx/video/fb.h>
#include <utils/Log.h>

namespace os {
namespace wm {

RootContainer::RootContainer() : mDisp(nullptr), mSyncMode(DSM_TIMER) {
#ifdef CONFIG_FB_SYNC
    mVsyncEvent = FBIO_WAITFORVSYNC;
#endif
    init();
}

RootContainer::~RootContainer() {
    // TODO: lvgl deinit
    mDisp = nullptr;
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
    lv_init();
    // TODO: custom init for porting

    lv_porting_init();

    mDisp = lv_disp_get_default();
    return mDisp ? true : false;
}

bool RootContainer::drawFrame() {
    lv_refr_now(mDisp);
    return true;
}

bool RootContainer::handleEvent(int fd, int events) {
    if (events == mVsyncEvent) {
        _lv_disp_refr_timer(NULL);
        return true;
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

} // namespace wm
} // namespace os
