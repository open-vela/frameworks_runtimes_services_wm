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

#define LOG_TAG "WMS:Node"

#include "WindowNode.h"

#include "../common/WindowUtils.h"
#include "wm/LayoutParams.h"

namespace os {
namespace wm {

static bool acquireDrawBuffer(lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc);
static bool releaseDrawBuffer(lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc);
static bool sendInputEvent(lv_mainwnd_metainfo_t* meta, lv_mainwnd_input_event_t* event);

#define ASSERT_NOT_NULLPTR(ptr) \
    if ((ptr == nullptr)) {     \
        return false;           \
    }

static inline WindowNode* toWindowNode(lv_mainwnd_metainfo_t* meta) {
    if (meta == NULL) {
        return NULL;
    }
    return static_cast<WindowNode*>(meta->info);
}

static inline void setWidgetMetaInfo(lv_obj_t* obj, void* data, bool enableInput) {
    if (enableInput) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }

    if (xmsLiteMode()) {
        return;
    }

    lv_mainwnd_metainfo_t meta;
    meta.acquire_buffer = acquireDrawBuffer;
    meta.release_buffer = releaseDrawBuffer;
    meta.send_input_event = enableInput ? sendInputEvent : nullptr;
    meta.on_destroy = nullptr;
    meta.info = data;
    lv_mainwnd_set_metainfo(obj, &meta);
}

WindowNode::WindowNode(WindowState* state, void* parent, const Rect& rect, bool enableInput,
                       int32_t format)
      : mState(state), mBuffer(nullptr), mClientScreen(nullptr) {
    mVisibility = LayoutParams::WINDOW_GONE;

    if (lv_obj_has_flag((lv_obj_t*)parent, LV_OBJ_FLAG_SCROLLABLE)) {
        lv_obj_clear_flag((lv_obj_t*)parent, LV_OBJ_FLAG_SCROLLABLE);
    }
    mWidget = lv_mainwnd_create((lv_obj_t*)parent);
    lv_obj_add_flag(mWidget, LV_OBJ_FLAG_HIDDEN);

    if (xmsLiteMode()) {
        mClientScreen = lv_obj_create(mWidget);
    }

    mColorFormat = getLvColorFormatType(format);
    setRect(rect);

    // init window meta information
    setWidgetMetaInfo(mWidget, this, enableInput);
    lv_mainwnd_update_flag(mWidget, LV_MAINWND_FLAG_DRAW_SCALE, false);
}

WindowNode::~WindowNode() {
    if (mWidget) lv_obj_del(mWidget);
    if (xmsLiteMode()) return;

    releaseBuffer();
}

void WindowNode::resetOpaque() {
    lv_obj_set_style_opa(mWidget, 0xFF, LV_PART_MAIN);
}

void WindowNode::setRect(const Rect& newRect) {
    if (mWidget == nullptr) return;

    int32_t left = newRect.getLeft();
    int32_t top = newRect.getTop();
    int32_t width = newRect.getWidth();
    int32_t height = newRect.getHeight();

    FLOGI("update node pos(%" PRId32 ", %" PRId32 "), size(%" PRId32 "x%" PRId32 ")", left, top,
          width, height);

    if (left != mRect.getLeft()) {
        lv_obj_set_x(mWidget, left);
    }

    if (top != mRect.getTop()) {
        lv_obj_set_y(mWidget, top);
    }

    if (width != mRect.getWidth()) {
        lv_obj_set_width(mWidget, width);
        lv_obj_set_style_transform_pivot_x(mWidget, width / 2, LV_PART_MAIN);
        if (xmsLiteMode()) {
            lv_obj_set_width(mClientScreen, width);
        }
    }

    if (height != mRect.getHeight()) {
        lv_obj_set_height(mWidget, height);
        lv_obj_set_style_transform_pivot_y(mWidget, height / 2, LV_PART_MAIN);
        if (xmsLiteMode()) {
            lv_obj_set_height(mClientScreen, height);
        }
    }
    mRect = newRect;
}

void WindowNode::setParent(void* parent) {
    FLOGI("update node parent");
    if (mWidget) {
        lv_obj_set_parent(mWidget, (lv_obj_t*)parent);
    }
}

uint32_t WindowNode::getSurfaceSize() {
    int bpp = lv_color_format_get_bpp(mColorFormat);
    return mRect.getWidth() * mRect.getHeight() * (bpp >> 3);
}

void WindowNode::syncClientVisibility(int32_t visibility) {
    if (mVisibility == visibility) return;

    bool newVisible = visibility == LayoutParams::WINDOW_VISIBLE;
    bool oldVisible = mVisibility == LayoutParams::WINDOW_VISIBLE;

    mVisibility = visibility;

    FLOGI("[%d] visibility %s -> %s", mState->getToken()->getClientPid(),
          LayoutParams::visibilityToName(mVisibility), LayoutParams::visibilityToName(visibility));

    if (!mWidget) return;

    /* if window need input, update window meta info*/
    if (mState && mState->needInput()) {
        setWidgetMetaInfo(mWidget, this, newVisible);

        if (xmsLiteMode()) return;

        /* check last message state only for multi-instance mode */
        if (!newVisible && oldVisible && mLastInputMsg.state != INPUT_MESSAGE_STATE_RELEASED) {
            InputMessage ie = mLastInputMsg;
            ie.state = INPUT_MESSAGE_STATE_RELEASED;
            bool ret = sendInputMessage(&ie);

            (void)ret; // maybe unused
            FLOGI("%p add mismatch message automatically for app: %s", this,
                  ret ? "success" : "failure");
        }
    }
}

void WindowNode::setVisibility(int32_t visibility) {
    syncClientVisibility(visibility);

    if (!xmsLiteMode()) {
        return;
    }
    FLOGI("update visibility to %s", LayoutParams::visibilityToName(visibility));

    if (!mWidget) return;

    switch (mVisibility) {
        case LayoutParams::WINDOW_VISIBLE:
            if (lv_obj_has_flag(mWidget, LV_OBJ_FLAG_HIDDEN)) {
                FLOGD("from HIDE to SHOW");
                lv_obj_clear_flag(mWidget, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_move_to_index(mWidget, -1);
            break;

        case LayoutParams::WINDOW_HOLD:
            if (lv_obj_has_flag(mWidget, LV_OBJ_FLAG_HIDDEN)) {
                FLOGD("from HOLD to SHOW");
                lv_obj_clear_flag(mWidget, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case LayoutParams::WINDOW_GONE:
            FLOGD("switch to HIDE");
            lv_obj_add_flag(mWidget, LV_OBJ_FLAG_HIDDEN);
            break;

        default:
            break;
    }
}

/* only for non-lite runmode */
static inline void initBufDsc(lv_mainwnd_buf_dsc_t* buf_dsc, BufferKey id, int32_t w, int32_t h,
                              uint32_t format, uint32_t size, void* data) {
    if (xmsLiteMode()) return;

    buf_dsc->id = id;
    buf_dsc->img_dsc.header.w = w;
    buf_dsc->img_dsc.header.h = h;
    buf_dsc->img_dsc.header.cf = format;
    buf_dsc->img_dsc.data_size = size;
    buf_dsc->img_dsc.data = (const uint8_t*)data;
}

// for lvgl mainwnd
static bool acquireDrawBuffer(lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc) {
    if (xmsLiteMode()) return false;

    ASSERT_NOT_NULLPTR(buf_dsc);

    WindowNode* node = toWindowNode(meta);
    ASSERT_NOT_NULLPTR(node);

    BufferItem* bufItem = node->acquireBuffer();
    if (bufItem == nullptr) {
        FLOGI("acquire buffer from state failure!");
        return false;
    }

    Rect& rect = node->getRect();

    initBufDsc(buf_dsc, bufItem->mKey, rect.getWidth(), rect.getHeight(), node->getColorFormat(),
               bufItem->mSize, bufItem->mBuffer);
    return true;
}

static bool releaseDrawBuffer(lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc) {
    if (xmsLiteMode()) return false;

    WindowNode* node = toWindowNode(meta);
    ASSERT_NOT_NULLPTR(node);

    if (buf_dsc) {
        node->releaseBuffer();
    }
    return false;
}

static bool sendInputEvent(lv_mainwnd_metainfo_t* meta, lv_mainwnd_input_event_t* event) {
    if (xmsLiteMode()) return false;

    if (event == nullptr) {
        FLOGW("input event: shouldn't send null event!");
        return false;
    }
    WindowNode* node = toWindowNode(meta);
    if (node == nullptr) {
        FLOGW("input event: no valid window, can't send it!");
        return false;
    }

    return node->sendInputMessage((const InputMessage*)event);
}

bool WindowNode::updateBuffer(BufferItem* item, Rect* rect, uint32_t seq) {
    if (xmsLiteMode()) return false;

    WM_PROFILER_BEGIN();

    bool result = false;
    lv_area_t area;
    lv_mainwnd_buf_dsc_t dsc;
    BufferItem* oldBuffer = mBuffer;

    mBuffer = item;
    if (rect) {
        area.x1 = rect->left;
        area.y1 = rect->top;
        area.y2 = rect->bottom;
        area.x2 = rect->right;
    }

    if (mBuffer) {
        initBufDsc(&dsc, mBuffer->mKey, mRect.getWidth(), mRect.getHeight(), getColorFormat(),
                   mBuffer->mSize, mBuffer->mBuffer);
        dsc.seq = seq;
        result = lv_mainwnd_update_buffer(mWidget, &dsc, rect ? &area : nullptr);
    } else {
        result = lv_mainwnd_update_buffer(mWidget, NULL, NULL);
    }

    FLOGD("%s from(0x%0" PRIx32 ") to(0x%0" PRIx32 ") seq=%" PRIu32 "\n",
          result ? "success" : "failure", oldBuffer ? oldBuffer->mKey : -1,
          mBuffer ? mBuffer->mKey : -1, seq);

    // need to reset buffer
    if (!result) {
        mBuffer = oldBuffer;
    } else if (oldBuffer && !mState->releaseBuffer(oldBuffer)) {
        FLOGW("releaseBuffer(%" PRId32 ") exception\n", oldBuffer->mKey);
        WM_PROFILER_END();
        return false;
    }
    WM_PROFILER_END();
    return result;
}

BufferItem* WindowNode::acquireBuffer() {
    if (xmsLiteMode()) return nullptr;

    if (!mBuffer) {
        mBuffer = mState->acquireBuffer();
    }
    return mBuffer;
}

bool WindowNode::releaseBuffer() {
    if (xmsLiteMode()) return false;

    if (mBuffer) {
        return mState->releaseBuffer(mBuffer);
    }
    return false;
}

bool WindowNode::sendInputMessage(const InputMessage* ie) {
    mLastInputMsg = *ie;
    return mState ? mState->sendInputMessage(ie) : false;
}

} // namespace wm

} // namespace os
