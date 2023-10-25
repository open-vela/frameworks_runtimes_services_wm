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

#define LOG_TAG "LVGLProxy"

#include "LVGLDriverProxy.h"

#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>

namespace os {
namespace wm {
static lv_display_t* _disp_init(LVGLDriverProxy* proxy);
static lv_indev_t* _indev_init(LVGLDriverProxy* proxy);

LVGLDriverProxy::LVGLDriverProxy(std::shared_ptr<BaseWindow> win)
      : UIDriverProxy(win),
        mIndev(NULL),
        mEventFd(-1),
        mLastEventState(LV_INDEV_STATE_RELEASED),
        mRenderMode(LV_DISP_RENDER_MODE_FULL) {
    mDisp = _disp_init(this);
    mDispW = mDisp->hor_res;
    mDispH = mDisp->ver_res;
    lv_display_set_default(mDisp);
}

LVGLDriverProxy::~LVGLDriverProxy() {
    if (mDisp) {
        lv_display_remove(mDisp);
        mDisp = NULL;
    }

    if (mIndev) {
        lv_indev_delete(mIndev);
        mIndev = NULL;
    }
    mEventFd = -1;
    mLastEventState = LV_INDEV_STATE_RELEASED;
}

void LVGLDriverProxy::drawFrame(BufferItem* bufItem) {
    FLOGD("");

    BufferItem* oldItem = getBufferItem();
    UIDriverProxy::drawFrame(bufItem);
    if (!bufItem) {
        return;
    }

    if (oldItem && mRenderMode == LV_DISP_RENDER_MODE_DIRECT) {
        uint32_t hor_res = lv_display_get_horizontal_resolution(mDisp);
        uint32_t ver_res = lv_display_get_vertical_resolution(mDisp);
        lv_area_t scr_area;
        scr_area.x1 = 0;
        scr_area.y1 = 0;
        scr_area.x2 = lv_display_get_horizontal_resolution(mDisp) - 1;
        scr_area.y2 = lv_display_get_vertical_resolution(mDisp) - 1;

        lv_draw_buf_copy(bufItem->mBuffer, hor_res, ver_res, &scr_area, oldItem->mBuffer, hor_res,
                         ver_res, &scr_area, lv_display_get_color_format(mDisp));
    }

    if (lv_display_get_default() != mDisp) {
        lv_display_set_default(mDisp);
    }
    _lv_display_refr_timer(NULL);
}

void LVGLDriverProxy::handleEvent() {
    if (mIndev) lv_indev_read(mIndev);
}

void* LVGLDriverProxy::getRoot() {
    return mDisp;
}

void* LVGLDriverProxy::getWindow() {
    lv_obj_t* screen = lv_display_get_screen_act(mDisp);
    if (screen) {
        uint32_t hor_res = lv_display_get_horizontal_resolution(mDisp);
        uint32_t ver_res = lv_display_get_vertical_resolution(mDisp);

        lv_obj_set_width(screen, hor_res);
        lv_obj_set_height(screen, ver_res);
    }
    return screen;
}

bool LVGLDriverProxy::enableInput(bool enable) {
    if (enable && !mIndev) {
        mIndev = _indev_init(this);
        return mIndev ? true : false;
    }

    if (mIndev) {
        lv_indev_enable(mIndev, enable);
        return true;
    }
    return false;
}

void LVGLDriverProxy::updateResolution(int32_t width, int32_t height) {
    lv_display_set_resolution(mDisp, width, height);
}

void LVGLDriverProxy::updateVisibility(bool visible) {
    if (visible) {
        if (!lv_display_is_invalidation_enabled(mDisp)) lv_display_enable_invalidation(mDisp, true);

        lv_area_t area;
        area.x1 = 0;
        area.y1 = 0;
        area.x2 = lv_display_get_horizontal_resolution(mDisp) - 1;
        area.y2 = lv_display_get_vertical_resolution(mDisp) - 1;
        _lv_inv_area(mDisp, &area);

    } else if (lv_display_is_invalidation_enabled(mDisp)) {
        lv_display_enable_invalidation(mDisp, false);
    }
}

static void _disp_flush_cb(lv_display_t* disp, const lv_area_t* area_p, uint8_t* color_p) {
    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_display_get_user_data(disp));
    if (proxy) {
        proxy->onQueueBuffer();

        Rect inv_rect = Rect(area_p->x1, area_p->y1, area_p->x2, area_p->y2);
        proxy->onRectCrop(inv_rect);
        FLOGD("%p display flush area (%d,%d)->(%d,%d)", proxy, area_p->x1, area_p->y1, area_p->x2,
              area_p->y2);
    }
    lv_display_flush_ready(disp);
}

static void _disp_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
        case LV_EVENT_RENDER_START: {
            LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_event_get_user_data(e));
            if (proxy == NULL) {
                return;
            }

            void* buffer = proxy->onDequeueBuffer();
            if (buffer) {
                FLOGD("%p render start", proxy);
                proxy->mDisp->buf_act = (uint8_t*)buffer;
            }
            break;
        }
        case LV_EVENT_REFR_REQUEST:      // for LV_DISP_RENDER_MODE_FULL
        case LV_EVENT_INVALIDATE_AREA: { // for LV_DISP_RENDER_MODE_DIRECT
            LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_event_get_user_data(e));
            if (proxy == NULL) {
                return;
            }

            if (code == LV_EVENT_INVALIDATE_AREA && !proxy->getBufferItem()) {
                /* need to invalidate the whole screen*/
                lv_display_t* disp = (lv_display_t*)lv_event_get_target(e);
                lv_area_t* area = (lv_area_t*)lv_event_get_param(e);
                if (area) {
                    area->x1 = 0;
                    area->y1 = 0;
                    area->x2 = lv_display_get_horizontal_resolution(disp) - 1;
                    area->y2 = lv_display_get_vertical_resolution(disp) - 1;
                }
            }

            bool periodic = lv_anim_get_timer()->paused ? false : true;
            if (proxy->onInvalidate(periodic)) {
                FLOGD("%p invalidate area", proxy);
            }
            break;
        }

        case LV_EVENT_RESOLUTION_CHANGED: {
            lv_display_t* disp = (lv_display_t*)lv_event_get_target(e);
            FLOGI("Resolution changed to (%dx%d)", disp->hor_res, disp->ver_res);

            lv_obj_t* act_screen = lv_display_get_screen_act(disp);
            if (act_screen) {
                lv_obj_set_width(act_screen, disp->hor_res);
                lv_obj_set_height(act_screen, disp->ver_res);
            }

            LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_event_get_user_data(e));
            if (proxy == NULL) {
                return;
            }
            WindowEventListener* listener = proxy->getEventListener();
            if (listener) {
                lv_coord_t oldW = proxy->mDispW;
                lv_coord_t oldH = proxy->mDispH;
                proxy->mDispW = disp->hor_res;
                proxy->mDispH = disp->ver_res;
                listener->onSizeChanged(disp->hor_res, disp->ver_res, oldW, oldH);
            }

            break;
        }

        case LV_EVENT_DELETE:
            FLOGD("try to delete window");
            break;

        default:
            break;
    }
}

static char _virt_disp_buffer[4];
static lv_display_t* _disp_init(LVGLDriverProxy* proxy) {
    static uint32_t width = 0, height = 0;
    if (width == 0) {
        WindowManager::getInstance()->getDisplayInfo(&width, &height);
    }

    lv_display_t* disp = lv_display_create(width, height);
    if (disp == NULL) {
        return NULL;
    }

    lv_timer_del(disp->refr_timer);
    disp->refr_timer = NULL;

    uint32_t buf_size = sizeof(_virt_disp_buffer);
    lv_display_set_draw_buffers(disp, _virt_disp_buffer, NULL, buf_size, proxy->renderMode());
    lv_display_set_flush_cb(disp, _disp_flush_cb);
    lv_event_add(&disp->event_list, _disp_event_cb, LV_EVENT_ALL, proxy);
    lv_display_set_user_data(disp, proxy);

    return disp;
}

static void _indev_read(lv_indev_t* drv, lv_indev_data_t* data) {
    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(drv->user_data);
    if (proxy == NULL) {
        return;
    }

    InputMessage message;
    bool ret = proxy->readEvent(&message);

    if (ret) {
        dumpInputMessage(&message);
        if (message.type == INPUT_MESSAGE_TYPE_POINTER) {
            if (message.state == INPUT_MESSAGE_STATE_PRESSED) {
                const lv_display_t* disp_drv = proxy->mDisp;
                lv_coord_t ver_max = disp_drv->ver_res - 1;
                lv_coord_t hor_max = disp_drv->hor_res - 1;

                data->point.x = LV_CLAMP(0, message.pointer.raw_x, hor_max);
                data->point.y = LV_CLAMP(0, message.pointer.raw_y, ver_max);
                proxy->mLastEventState = LV_INDEV_STATE_PRESSED;
            } else if (message.state == INPUT_MESSAGE_STATE_RELEASED) {
                proxy->mLastEventState = LV_INDEV_STATE_RELEASED;
            }

            data->continue_reading = true;
        }
    }
    data->state = proxy->mLastEventState;
}

static lv_indev_t* _indev_init(LVGLDriverProxy* proxy) {
    lv_indev_t* indev = lv_indev_create();
    if (indev == NULL) {
        return NULL;
    }
    indev->type = LV_INDEV_TYPE_POINTER;
    indev->read_cb = _indev_read;
    indev->user_data = proxy;

    lv_timer_del(indev->read_timer);
    indev->read_timer = NULL;
    return indev;
}

} // namespace wm
} // namespace os
