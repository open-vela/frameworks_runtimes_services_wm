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

#include <lvgl/lvgl.h>

#include "ext/lvgl_inst.h"

#if LV_VERSION_CHECK(9, 0, 0)
#include <lvgl/src/lvgl_private.h>
#else
#include <lv_porting/lv_porting.h>
#endif

#include "WindowManagerService.h"
#include "WindowUtils.h"

namespace os {
namespace wm {

RootContainer::RootContainer(WindowManagerService* service)
      : mService(service), mDisp(NULL), mIndev(NULL), mInputFd(-1), mDispFd(-1) {
    init();
}

RootContainer::~RootContainer() {
    if (mDispFd > 0) {
        mService->unregisterFd(mDispFd);
        close(mDispFd);
        mDispFd = -1;
    }
    if (mInputFd > 0) {
        mService->unregisterFd(mInputFd);
        mInputFd = -1;
    }

    lv_anim_del_all();
    if (mDisp) {
        lv_disp_remove(mDisp);
        mDisp = NULL;
    }
    if (mIndev) {
        lv_indev_delete(mIndev);
        mIndev = NULL;
    }
    mService = nullptr;
    lv_deinit();
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

static inline int vsyncCallback(int fd, int events, void* data) {
    RootContainer* container = static_cast<RootContainer*>(data);
    if (container) {
        container->processVsyncEvent();
    }
    return 1;
}

static inline int inputCallback(int fd, int events, void* data) {
    RootContainer* container = static_cast<RootContainer*>(data);
    if (container) {
        container->processInputEvent();
    }
    return 1;
}

void RootContainer::processVsyncEvent() {
    WM_PROFILER_BEGIN();

    _lv_disp_get_refr_timer(NULL);
    if (mService && mService->responseVsync()) {
        uint32_t delay = lv_timer_get_time_until_next();
        if (delay != LV_NO_TIMER_READY) {
            FLOGI("resume ui timer!");
            mService->postTimerMessage(delay);
        }
    }

    WM_PROFILER_END();
}

int32_t RootContainer::handleTimer() {
    WM_PROFILER_BEGIN();
    uint32_t delay = lv_timer_handler();
    int32_t result = delay == LV_NO_TIMER_READY ? -1 : delay;
    WM_PROFILER_END();
    return result;
}

void RootContainer::processInputEvent() {
    WM_PROFILER_BEGIN();
#if LV_VERSION_CHECK(9, 0, 0)
    lv_indev_read(mIndev);
#endif
    WM_PROFILER_END();
}

bool RootContainer::init() {
    lv_init();
    lv_tick_set_cb(millis);

#if LV_VERSION_CHECK(9, 0, 0)

#if LV_USE_NUTTX_FBDEV
    mDisp = lv_nuttx_fbdev_create();
    lv_nuttx_fbdev_set_file(mDisp, CONFIG_LV_FBDEV_INTERFACE_DEFAULT_DEVICEPATH);

    mDispFd = open(CONFIG_LV_FBDEV_INTERFACE_DEFAULT_DEVICEPATH, O_WRONLY);
    // if (mDispFd > 0) {
    //     if (mService->registerFd(fd, POLL_EVENT_OUTPUT, vsyncCallback, (void*)this)) {
    //         lv_timer_del(mDisp->refr_timer);
    //         mDisp->refr_timer = NULL;
    //     }
    // }
#endif

    lv_timer_t* timer = _lv_disp_get_refr_timer(mDisp);
    if (timer) {
        lv_timer_set_period(timer, DEF_REFR_PERIOD);
    }

#if LV_USE_NUTTX_TOUCHSCREEN
    mIndev = lv_nuttx_touchscreen_create(CONFIG_LV_TOUCHPAD_INTERFACE_DEFAULT_DEVICEPATH);

    mInputFd = (int32_t)lv_indev_get_user_data(mIndev);
    if (mInputFd > 0 &&
        mService->registerFd(mInputFd, POLL_EVENT_INPUT, inputCallback, (void*)mIndev)) {
        lv_timer_del(mIndev->read_timer);
        mIndev->read_timer = NULL;
    }
#endif

#else
    lv_porting_init();
    mDisp = lv_disp_get_default();
#endif

    return mDisp ? true : false;
}

bool RootContainer::getDisplayInfo(DisplayInfo* info) {
    if (info) {
        info->width = lv_disp_get_hor_res(mDisp);
        info->height = lv_disp_get_ver_res(mDisp);
        return true;
    }

    return false;
}

static lv_anim_t* toast_fade_in(lv_obj_t* obj, uint32_t time, uint32_t delay);
static lv_anim_t* toast_fade_out(lv_obj_t* obj, uint32_t time, uint32_t delay);

void RootContainer::showToast(const char* text, uint32_t duration) {
    lv_obj_t* label = lv_label_create(lv_disp_get_layer_sys(mDisp));
    lv_label_set_text(label, text);
    lv_obj_set_style_bg_opa(label, LV_OPA_80, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_pad_all(label, 5, 0);
    lv_obj_set_style_radius(label, 10, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_user_data(label, (void*)duration);

    toast_fade_in(label, 500, 100);
}

static void toast_fade_anim_cb(void* obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
}

static void toast_fade_in_anim_ready(lv_anim_t* a) {
    lv_obj_remove_local_style_prop((lv_obj_t*)(a->var), LV_STYLE_OPA, 0);
    lv_obj_t* target = (lv_obj_t*)(a->var);
    uint32_t duration = (uint32_t)lv_obj_get_user_data(target);
    toast_fade_out(target, lv_anim_get_time(a), duration);
}

static void fade_out_anim_ready(lv_anim_t* a) {
    lv_obj_del((lv_obj_t*)(a->var));
}

static lv_anim_t* toast_fade_in(lv_obj_t* obj, uint32_t time, uint32_t delay) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, toast_fade_anim_cb);
    lv_anim_set_ready_cb(&a, toast_fade_in_anim_ready);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    return lv_anim_start(&a);
}

static lv_anim_t* toast_fade_out(lv_obj_t* obj, uint32_t time, uint32_t delay) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, lv_obj_get_style_opa(obj, 0), LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, toast_fade_anim_cb);
    lv_anim_set_ready_cb(&a, fade_out_anim_ready);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    return lv_anim_start(&a);
}

} // namespace wm
} // namespace os
