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
#include <lvgl/src/lvgl_private.h>

#include "WindowManagerService.h"
#include "WindowUtils.h"

namespace os {
namespace wm {

RootContainer::RootContainer(WindowManagerService* service)
      : mService(service), mDisp(NULL), mVsyncTimer(NULL), mUvData(NULL) {
    init();
}

RootContainer::~RootContainer() {
    if (mVsyncTimer) lv_timer_del(mVsyncTimer);

    lv_anim_del_all();
#if LV_USE_NUTTX_LIBUV
    lv_nuttx_uv_deinit(&mUvData);
#endif

    if (mDisp) {
        lv_disp_remove(mDisp);
        mDisp = NULL;
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

static void vsyncCallback(lv_timer_t* tmr) {
    RootContainer* container = static_cast<RootContainer*>(lv_timer_get_user_data(tmr));
    if (container) {
        container->processVsyncEvent();
    }
}

static void resetVsyncTimer(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_REFR_FINISH) {
        WM_PROFILER_BEGIN();
        lv_timer_t* timer = (lv_timer_t*)(lv_event_get_user_data(e));
        WM_PROFILER_END();

        if (!timer->paused) {
            lv_timer_reset(timer);
            if (timer->timer_cb) timer->timer_cb(timer);
        }
    }
}

void RootContainer::enableVsync(bool enable) {
    WM_PROFILER_BEGIN();
    if (mVsyncTimer) {
        if (enable && mVsyncTimer->paused) {
            lv_timer_resume(mVsyncTimer);
        } else if (!enable && !mVsyncTimer->paused) {
            lv_timer_pause(mVsyncTimer);
        }
    }
    WM_PROFILER_END();
}

void RootContainer::processVsyncEvent() {
    WM_PROFILER_BEGIN();
    if (mService) mService->responseVsync();

    WM_PROFILER_END();
}

bool RootContainer::init() {
    lv_init();

#if LV_USE_NUTTX
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;

    info.fb_path = CONFIG_LV_FBDEV_INTERFACE_DEFAULT_DEVICEPATH;
    info.input_path = CONFIG_LV_TOUCHPAD_INTERFACE_DEFAULT_DEVICEPATH;
    lv_nuttx_init(&info, &result);
    mDisp = result.disp;
#if defined(CONFIG_UINPUT_TOUCH)
    lv_nuttx_touchscreen_create("/dev/utouch");
#endif
#if LV_USE_NUTTX_LIBUV
    lv_nuttx_uv_t uv_info = {
            .loop = mService->getUvLooper(),
            .disp = result.disp,
            .indev = result.indev,
    };
    mUvData = lv_nuttx_uv_init(&uv_info);
#endif
    mVsyncTimer = lv_timer_create(vsyncCallback, DEF_REFR_PERIOD, this);
    lv_event_add(&mDisp->event_list, resetVsyncTimer, LV_EVENT_REFR_FINISH, mVsyncTimer);
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
