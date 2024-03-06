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

#include "../common/WindowUtils.h"
#include "WindowManagerService.h"

namespace os {
namespace wm {

RootContainer::RootContainer(DeviceEventListener* listener, uv_loop_t* loop)
      : mListener(listener),
        mDisp(nullptr),
#ifdef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
        mVsyncEnabled(true),
#else
        mVsyncTimer(nullptr),
#endif
        mUvData(nullptr),
        mUvLoop(loop) {
    init();
}

RootContainer::~RootContainer() {
    LV_GLOBAL_DEFAULT()->user_data = nullptr;
#ifndef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
    if (mVsyncTimer) lv_timer_del(mVsyncTimer);
#endif

    lv_anim_del_all();

    lv_nuttx_uv_deinit(&mUvData);
    mUvData = nullptr;
    mUvLoop = nullptr;

    if (mDisp) {
        lv_disp_remove(mDisp);
        mDisp = nullptr;
    }

    mListener = nullptr;

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

#ifdef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
static void vsyncEventReceived(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VSYNC) {
        WM_PROFILER_BEGIN();

        RootContainer* container = reinterpret_cast<RootContainer*>(lv_event_get_user_data(e));
        if (container && container->vsyncEnabled()) {
            container->processVsyncEvent();
        }

        WM_PROFILER_END();
    }
}
#else
static void vsyncCallback(lv_timer_t* tmr) {
    RootContainer* container = static_cast<RootContainer*>(lv_timer_get_user_data(tmr));
    if (container) {
        container->processVsyncEvent();
    }
}

static void resetVsyncTimer(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_REFR_READY) {
        WM_PROFILER_BEGIN();
        lv_timer_t* timer = (lv_timer_t*)(lv_event_get_user_data(e));
        WM_PROFILER_END();

        if (!timer->paused) {
            lv_timer_reset(timer);
            if (timer->timer_cb) timer->timer_cb(timer);
        }
    }
}
#endif

void RootContainer::enableVsync(bool enable) {
    WM_PROFILER_BEGIN();
#ifdef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
    mVsyncEnabled = enable;
#else
    if (mVsyncTimer) {
        if (enable && mVsyncTimer->paused) {
            lv_timer_resume(mVsyncTimer);
        } else if (!enable && !mVsyncTimer->paused) {
            lv_timer_pause(mVsyncTimer);
        }
    }
#endif
    WM_PROFILER_END();
}

void RootContainer::processVsyncEvent() {
    WM_PROFILER_BEGIN();
    if (mListener) {
        mListener->responseVsync();
    }

    WM_PROFILER_END();
}

static void monitor_indev_read(lv_indev_t* indev, lv_indev_data_t* data) {
    if (!data) return;

    RootContainer* container = reinterpret_cast<RootContainer*>(LV_GLOBAL_DEFAULT()->user_data);
    if (container) container->readInput(indev, data);
}

void RootContainer::readInput(lv_indev_t* indev, lv_indev_data_t* data) {
    lv_indev_read_cb_t read_cb = (lv_indev_read_cb_t)lv_indev_get_user_data(indev);
    if (!read_cb) return;

    read_cb(indev, data);

    if (!mListener) return;

    int type = lv_indev_get_type(indev);
    InputMessage msg;
    msg.type = (InputMessageType)type;
    msg.state = (InputMessageState)data->state;

    switch (type) {
        case LV_INDEV_TYPE_POINTER:
            msg.pointer.x = msg.pointer.raw_x = data->point.x;
            msg.pointer.y = msg.pointer.raw_y = data->point.y;
            break;
        case LV_INDEV_TYPE_KEYPAD:
            msg.keypad.key_code = data->key;
            break;
        default:
            return;
    }

    mListener->responseInput(&msg);
}

bool RootContainer::init() {
    lv_init();

#if LV_USE_NUTTX
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;

    lv_nuttx_dsc_init(&info);
    info.fb_path = CONFIG_LV_FBDEV_INTERFACE_DEFAULT_DEVICEPATH;
    info.input_path = CONFIG_LV_TOUCHPAD_INTERFACE_DEFAULT_DEVICEPATH;

    lv_nuttx_init(&info, &result);
    mDisp = result.disp;
    lv_nuttx_uv_t uv_info = {
            .loop = mUvLoop,
            .disp = result.disp,
            .indev = result.indev,
    };
    mUvData = lv_nuttx_uv_init(&uv_info);

#ifdef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
    lv_display_add_event_cb(mDisp, vsyncEventReceived, LV_EVENT_VSYNC, this);
    FLOGW("Now server by event");
#else
    mVsyncTimer = lv_timer_create(vsyncCallback, LV_DEF_REFR_PERIOD, this);
    lv_display_add_event_cb(mDisp, resetVsyncTimer, LV_EVENT_REFR_READY, mVsyncTimer);
#endif

    if (mListener) {
        LV_GLOBAL_DEFAULT()->user_data = this;

        lv_indev_set_user_data(result.indev, (void*)lv_indev_get_read_cb(result.indev));
        lv_indev_set_read_cb(result.indev, monitor_indev_read);
    }
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
    uint32_t duration = (uintptr_t)lv_obj_get_user_data(target);
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
