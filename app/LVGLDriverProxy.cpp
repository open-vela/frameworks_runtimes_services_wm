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

#if LV_VERSION_CHECK(9, 0, 0)
#include <lvgl/src/lvgl_private.h>
#else
#include <lv_porting/lv_porting.h>
#endif

#include <lvgl/lvgl.h>

namespace os {
namespace wm {
static lv_disp_t* _disp_init(LVGLDriverProxy* proxy);
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
    lv_disp_set_default(mDisp);
}

LVGLDriverProxy::~LVGLDriverProxy() {
    if (mDisp) {
        lv_disp_remove(mDisp);
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
        lv_coord_t stride =
                lv_draw_buf_width_to_stride(lv_disp_get_hor_res(mDisp), mDisp->color_format);
        lv_area_t scr_area;
        scr_area.x1 = 0;
        scr_area.y1 = 0;
        scr_area.x2 = lv_disp_get_hor_res(mDisp) - 1;
        scr_area.y2 = lv_disp_get_ver_res(mDisp) - 1;

        lv_draw_buf_copy(bufItem->mBuffer, stride, &scr_area, oldItem->mBuffer, stride, &scr_area,
                         mDisp->color_format);
    }

    if (lv_disp_get_default() != mDisp) {
        lv_disp_set_default(mDisp);
    }
    _lv_disp_refr_timer(NULL);
}

void LVGLDriverProxy::handleEvent() {
#if LV_VERSION_CHECK(9, 0, 0)
    if (mIndev) lv_indev_read(mIndev);
#endif
}

void* LVGLDriverProxy::getRoot() {
    return mDisp;
}

void* LVGLDriverProxy::getWindow() {
    return lv_disp_get_scr_act(mDisp);
}

#if !LV_VERSION_CHECK(9, 0, 0)

void LVGLDriverProxy::updateResolution(int32_t width, int32_t height) {}

static lv_disp_t* _disp_init(LVGLDriverProxy* proxy) {
    return NULL;
}

bool LVGLDriverProxy::enableInput(bool enable) {
    return false;
}
#else

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
    lv_disp_set_res(mDisp, width, height);
}

static void _disp_flush_cb(lv_disp_t* disp, const lv_area_t* area_p, uint8_t* color_p) {
    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_disp_get_user_data(disp));
    if (proxy) {
        proxy->onQueueBuffer();

        Rect inv_rect = Rect(area_p->x1, area_p->y1, area_p->x2, area_p->y2);
        proxy->onRectCrop(inv_rect);
        FLOGD("%p display flush area (%d,%d)->(%d,%d)", proxy, area_p->x1, area_p->y1, area_p->x2,
              area_p->y2);
    }
    lv_disp_flush_ready(disp);
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
                lv_disp_t* disp = (lv_disp_t*)lv_event_get_target(e);
                lv_area_t* area = (lv_area_t*)lv_event_get_param(e);
                if (area) {
                    area->x1 = 0;
                    area->y1 = 0;
                    area->x2 = lv_disp_get_hor_res(disp) - 1;
                    area->y2 = lv_disp_get_ver_res(disp) - 1;
                }
            }

            bool periodic = lv_anim_get_timer()->paused ? false : true;
            if (proxy->onInvalidate(periodic)) {
                FLOGD("%p invalidate area", proxy);
            }
            break;
        }

        case LV_EVENT_RESOLUTION_CHANGED: {
            lv_disp_t* disp = (lv_disp_t*)lv_event_get_target(e);
            FLOGI("Resolution changed to (%dx%d)", disp->hor_res, disp->ver_res);

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
static lv_disp_t* _disp_init(LVGLDriverProxy* proxy) {
    static uint32_t width = 0, height = 0;
    if (width == 0) {
        WindowManager::getInstance()->getDisplayInfo(&width, &height);
    }

    lv_disp_t* disp = lv_disp_create(width, height);
    if (disp == NULL) {
        return NULL;
    }

    lv_timer_del(disp->refr_timer);
    disp->refr_timer = NULL;

    uint32_t buf_size = sizeof(_virt_disp_buffer);
    lv_disp_set_draw_buffers(disp, _virt_disp_buffer, NULL, buf_size, proxy->renderMode());
    lv_disp_set_flush_cb(disp, _disp_flush_cb);
    lv_event_add(&disp->event_list, _disp_event_cb, LV_EVENT_ALL, proxy);
    lv_disp_set_user_data(disp, proxy);

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
                const lv_disp_t* disp_drv = proxy->mDisp;
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
#endif

} // namespace wm
} // namespace os
