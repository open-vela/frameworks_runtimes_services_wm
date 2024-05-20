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

#include "../common/WindowUtils.h"

namespace os {
namespace wm {
static lv_display_t* _disp_init(LVGLDriverProxy* proxy, uint32_t width, uint32_t height,
                                uint32_t cf);
static lv_indev_t* _indev_init(LVGLDriverProxy* proxy);

LVGLDrawBuffer::LVGLDrawBuffer(void* rawBuffer, uint32_t width, uint32_t height,
                               lv_color_format_t cf, uint32_t size) {
    uint32_t stride = lv_draw_buf_width_to_stride(width, cf);
    lv_draw_buf_init(&mDrawBuffer, width, height, cf, stride, rawBuffer, size);
}

LVGLDrawBuffer::~LVGLDrawBuffer() {
    FLOGD("");
}

LVGLDriverProxy::LVGLDriverProxy(std::shared_ptr<BaseWindow> win)
      : UIDriverProxy(win),
        mLastEventState(LV_INDEV_STATE_RELEASED),
        mIndev(NULL),
        mRenderMode(CONFIG_APP_WINDOW_RENDER_MODE),
        mAllAreaDirty(true),
        mPrevBuffer(NULL) {
    lv_color_format_t cf = getLvColorFormatType(win->getLayoutParams().mFormat);
    auto wm = win->getWindowManager();
    uint32_t width = 0, height = 0;
    wm->getDisplayInfo(&width, &height);

    /*set proxy information*/
    mDisp = _disp_init(this, width, height, cf);
    mDispW = mDisp->hor_res;
    mDispH = mDisp->ver_res;
    mDummyBuffer = mDisp->buf_1;

    lv_display_set_default(mDisp);
}

LVGLDriverProxy::~LVGLDriverProxy() {
    FLOGD("");
    if (mDisp) {
        lv_display_delete(mDisp);
        mDisp = NULL;
    }

    if (mIndev) {
        lv_indev_delete(mIndev);
        mIndev = NULL;
    }

    mDrawBuffers.clear();

    if (mDummyBuffer) {
        lv_draw_buf_destroy(mDummyBuffer);
    }

    mLastEventState = LV_INDEV_STATE_RELEASED;
}

void LVGLDriverProxy::drawFrame(BufferItem* bufItem) {
    UIDriverProxy::drawFrame(bufItem);
    if (!bufItem) {
        FLOGI("buffer is invalid");
        return;
    }
    if (lv_display_get_default() != mDisp) {
        lv_display_set_default(mDisp);
    }
    _lv_display_refr_timer(NULL);
    mAllAreaDirty = false;
}

void LVGLDriverProxy::onRenderStart() {
    void* buffer = onDequeueBuffer();
    if (!buffer) {
        FLOGE("no valid render buffer");
        return;
    }

    mDisp->buf_act = (lv_draw_buf_t*)buffer;

    if (mRenderMode == LV_DISPLAY_RENDER_MODE_DIRECT && mPrevBuffer && !mAllAreaDirty) {
        lv_area_t area = {0, 0, mDispW - 1, mDispH - 1};
        lv_draw_buf_copy((lv_draw_buf_t*)buffer, &area, (lv_draw_buf_t*)(mPrevBuffer->mUserData),
                         &area);
    }
}

void LVGLDriverProxy::onRenderEnd() {
    onQueueBuffer();
    if (mRenderMode == LV_DISPLAY_RENDER_MODE_DIRECT) {
        mPrevBuffer = getBufferItem();

        lv_area_t area;
        lv_display_get_dirty_area(mDisp, &area);
        Rect inv_rect = Rect(area.x1, area.y1, area.x2, area.y2);
        onRectCrop(inv_rect);
    }
}

void LVGLDriverProxy::onResolutionChanged(int32_t width, int32_t height) {
    FLOGI("Resolution changed from(%" PRId32 "x%" PRId32 ") to (%" PRId32 "x%" PRId32 ")", mDispW,
          mDispH, width, height);

    int32_t oldW = mDispW;
    int32_t oldH = mDispH;

    mDispW = width;
    mDispH = height;

    WindowEventListener* listener = getEventListener();
    if (listener) {
        listener->onSizeChanged(width, height, oldW, oldH);
    }
}

bool LVGLDriverProxy::updateDirtyArea(const lv_area_t* area) {
    if (mRenderMode != LV_DISPLAY_RENDER_MODE_DIRECT) return false;

    if (!area) return false;

    if (!getBufferItem() ||
        (lv_area_get_width(area) >= mDispW * 3 >> 2 &&
         lv_area_get_height(area) >= mDispH * 3 >> 2)) {
        mAllAreaDirty = true;
        return true;
    }
    return false;
}

void LVGLDriverProxy::handleEvent() {
    if (mIndev) lv_indev_read(mIndev);
}

void* LVGLDriverProxy::getRoot() {
    return mDisp;
}

void* LVGLDriverProxy::getWindow() {
    return lv_display_get_screen_active(mDisp);
}

void LVGLDriverProxy::setInputMonitor(InputMonitor* monitor) {
    UIDriverProxy::setInputMonitor(monitor);

    if (monitor && !mIndev) {
        mIndev = _indev_init(this);
    }

    if (mIndev) {
        lv_indev_enable(mIndev, monitor ? true : false);
    }
}

void* LVGLDriverProxy::onDequeueBuffer() {
    BufferItem* item = getBufferItem();
    if (item == nullptr) return nullptr;

    if (item->mUserData == nullptr) {
        void* buffer = UIDriverProxy::onDequeueBuffer();
        if (buffer) {
            FLOGI("%p init draw buffer", this);
            lv_color_format_t cf = lv_display_get_color_format(mDisp);
            auto drawBuffer =
                    std::make_shared<LVGLDrawBuffer>(buffer, mDispW, mDispH, cf, item->mSize);
            mDrawBuffers.push_back(drawBuffer);
            item->mUserData = drawBuffer->getDrawBuffer();
        }
    }

    return item->mUserData;
}

void LVGLDriverProxy::resetBuffer() {
    mPrevBuffer = NULL;
    mDisp->buf_act = mDummyBuffer;
    mDrawBuffers.clear();
    UIDriverProxy::resetBuffer();
}

void LVGLDriverProxy::updateResolution(int32_t width, int32_t height, uint32_t format) {
    lv_color_format_t color_format = getLvColorFormatType(format);
    FLOGI("%p update resolution (%" PRId32 "x%" PRId32 ") format %" PRId32 "->%d", this, width,
          height, format, color_format);

    lv_display_set_resolution(mDisp, width, height);
    lv_display_set_color_format(mDisp, color_format);
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

static void _disp_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color) {
    if (!lv_display_flush_is_last(disp)) {
        lv_display_flush_ready(disp);
        return;
    }

    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_display_get_user_data(disp));
    if (proxy) {
        proxy->onRenderEnd();
    }

    lv_display_flush_ready(disp);
}

#define CHECK_PROXY_OBJECT(e)                                                               \
    LVGLDriverProxy* proxy = reinterpret_cast<LVGLDriverProxy*>(lv_event_get_user_data(e)); \
    if (proxy == NULL) {                                                                    \
        FLOGI("proxy is invalid");                                                          \
        return;                                                                             \
    }

static void _disp_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
        case LV_EVENT_RENDER_START: {
            CHECK_PROXY_OBJECT(e);
            proxy->onRenderStart();
            break;
        }

        /* for FULL & DIRECT render mode */
        case LV_EVENT_REFR_REQUEST: {
            CHECK_PROXY_OBJECT(e);
            bool periodic = lv_anim_get_timer()->paused ? false : true;
            if (proxy->onInvalidate(periodic)) {
                FLOGD("%p invalidate area", proxy);
            }
            break;
        }

        case LV_EVENT_INVALIDATE_AREA: {
            CHECK_PROXY_OBJECT(e);
            lv_area_t* area = (lv_area_t*)lv_event_get_param(e);
            if (proxy->updateDirtyArea(area)) {
                lv_display_t* disp = (lv_display_t*)lv_event_get_target(e);
                int width = lv_display_get_horizontal_resolution(disp);
                int height = lv_display_get_vertical_resolution(disp);
                area->x1 = 0;
                area->y1 = 0;
                area->x2 = width - 1;
                area->y2 = height - 1;
            }
            break;
        }

        case LV_EVENT_RESOLUTION_CHANGED: {
            CHECK_PROXY_OBJECT(e);
            lv_display_t* disp = (lv_display_t*)lv_event_get_target(e);
            if (disp) proxy->onResolutionChanged(disp->hor_res, disp->ver_res);
            break;
        }

        case LV_EVENT_VSYNC_REQUEST: {
            CHECK_PROXY_OBJECT(e);
            void* param = lv_event_get_param(e);
            proxy->onFBVsyncRequest(param ? true : false);
            break;
        }

        case LV_EVENT_DELETE:
            FLOGD("try to delete window");
            break;

        default:
            break;
    }
}

static lv_display_t* _disp_init(LVGLDriverProxy* proxy, uint32_t width, uint32_t height,
                                uint32_t cf) {
    lv_display_t* disp = lv_display_create(width, height);
    if (disp == NULL) {
        return NULL;
    }

    lv_timer_del(disp->refr_timer);
    disp->refr_timer = NULL;

    /*for temporary placeholder buffer*/
    uint32_t dummyWidth = 1, dummyHeight = 1;
    uint32_t bufStride = lv_draw_buf_width_to_stride(dummyWidth, cf);

    FLOGI("size(%" LV_PRIu32 "*%" LV_PRIu32 ")", dummyWidth, dummyHeight);
    lv_draw_buf_t* dummyBuffer = lv_draw_buf_create(dummyWidth, dummyHeight, cf, bufStride);
    lv_display_set_draw_buffers(disp, dummyBuffer, NULL);
    lv_display_set_render_mode(disp, (lv_display_render_mode_t)proxy->renderMode());

    lv_display_set_flush_cb(disp, _disp_flush_cb);
    lv_event_add(&disp->event_list, _disp_event_cb, LV_EVENT_ALL, proxy);
    lv_display_set_user_data(disp, proxy);

    lv_obj_t* screen = lv_display_get_screen_active(disp);
    if (screen) {
        lv_obj_set_width(screen, width);
        lv_obj_set_height(screen, height);
    }

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
                const lv_display_t* disp_drv = (lv_display_t*)(proxy->getRoot());
                int32_t ver_max = disp_drv->ver_res - 1;
                int32_t hor_max = disp_drv->hor_res - 1;

                data->point.x = LV_CLAMP(0, message.pointer.x, hor_max);
                data->point.y = LV_CLAMP(0, message.pointer.y, ver_max);
                proxy->setLastEventState(LV_INDEV_STATE_PRESSED);
            } else if (message.state == INPUT_MESSAGE_STATE_RELEASED) {
                proxy->setLastEventState(LV_INDEV_STATE_RELEASED);
            }

            data->continue_reading = false;
        }
    }
    data->state = proxy->getLastEventState();
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

void LVGLDriverProxy::vsyncPollEvent(vector<std::shared_ptr<BaseWindow>> listeners) {
    for (const auto& win : listeners) {
        lv_display_send_vsync_event((lv_display_t*)(win->getNativeDisplay()), NULL);
    }
}

void LVGLDriverProxy::init() {
    lv_init();
#if LV_USE_NUTTX
    lv_nuttx_init(NULL, NULL);
#endif

#ifdef CONFIG_LVGL_EXTENSION
    lv_ext_init();
#endif
}

void LVGLDriverProxy::deinit() {
#ifdef CONFIG_LVGL_EXTENSION
    lv_ext_deinit();
#endif
    lv_deinit();
}

} // namespace wm
} // namespace os
