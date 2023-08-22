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

#if LVGL_VERSION_MAJOR >= 9
#include "wm/UIInstance.h"
#else
#include <lv_porting/lv_porting.h>
#endif

#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>

namespace os {
namespace wm {
static lv_disp_t* _disp_init(LVGLDriverProxy* proxy);

LVGLDriverProxy::LVGLDriverProxy(std::shared_ptr<BaseWindow> win)
      : UIDriverProxy(win), mDisp(nullptr), mIndev(nullptr) {
    initUIInstance();
}

LVGLDriverProxy::~LVGLDriverProxy() {
    UIDeinit();
}

bool LVGLDriverProxy::initUIInstance() {
    UIInit();
    mDisp = _disp_init(this);
    return mDisp ? true : false;
}

void LVGLDriverProxy::drawFrame(BufferItem* bufItem) {
    BufferItem* oldItem = getBufferItem();
    if (oldItem) {
        // TODO: copy old item buffer to current buffer
    }

    UIDriverProxy::drawFrame(bufItem);
    FLOGD("");

    lv_refr_now(mDisp);
}

void LVGLDriverProxy::handleEvent(InputMessage& message) {
    // TODO: switch message to indev event
    dumpInputMessage(&message);
}

void* LVGLDriverProxy::getRoot() {
    return mDisp;
}

void* LVGLDriverProxy::getWindow() {
    return lv_disp_get_scr_act(mDisp);
}

#if LVGL_VERSION_MAJOR < 9

void LVGLDriverProxy::updateResolution(int32_t width, int32_t height) {}

static lv_disp_t* _disp_init(LVGLDriverProxy* proxy) {
    return NULL;
}

#else

void LVGLDriverProxy::updateResolution(int32_t width, int32_t height) {
    lv_disp_set_res(mDisp, width, height);
}

static void _disp_flush_cb(lv_disp_t* disp, const lv_area_t* area_p, uint8_t* color_p) {
    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_disp_get_user_data(disp));
    if (proxy) {
        proxy->onQueueBuffer();

        Rect inv_rect = Rect(area_p->x1, area_p->y1, area_p->x2, area_p->y2);
        proxy->onRectCrop(inv_rect);
        FLOGD("display flush area(%dx%d -> %dx%d)", area_p->x1, area_p->y1, area_p->x2, area_p->y2);
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
                FLOGD("Render Start");
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

            bool periodic = lv_anim_get_timer()->paused ? false : true;
            if (proxy->onInvalidate(periodic)) {
                FLOGD("Invalidate area");
            }
            break;
        }

        case LV_EVENT_RESOLUTION_CHANGED:
            FLOGD("Resolution changed");
            break;

        default:
            break;
    }
}

static lv_disp_t* _disp_init(LVGLDriverProxy* proxy) {
    const int width = 1, height = 1;

    lv_disp_t* disp = lv_disp_create(1, 1);
    if (disp == NULL) {
        return NULL;
    }

    uint32_t px_size = lv_color_format_get_size(lv_disp_get_color_format(disp));
    uint32_t buf_size = width * height * px_size;

    void* draw_buf = lv_malloc(buf_size);
    if (draw_buf == NULL) {
        LV_LOG_ERROR("display draw_buf malloc failed");
        return NULL;
    }

    lv_disp_render_mode_t render_mode = LV_DISP_RENDER_MODE_FULL;
    // LV_DISP_RENDER_MODE_DIRECT
    lv_disp_set_draw_buffers(disp, draw_buf, NULL, buf_size, render_mode);
    lv_disp_set_flush_cb(disp, _disp_flush_cb);
    lv_event_add(&disp->event_list, _disp_event_cb, LV_EVENT_ALL, proxy);
    lv_disp_set_user_data(disp, proxy);

    return disp;
}

#endif

} // namespace wm
} // namespace os
