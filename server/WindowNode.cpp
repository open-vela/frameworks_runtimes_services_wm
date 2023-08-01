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

#include "WindowNode.h"

namespace os {
namespace wm {

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

static inline void initBufDsc(lv_mainwnd_buf_dsc_t* buf_dsc, BufferKey id, int32_t w, int32_t h,
                              uint32_t size, void* data) {
    buf_dsc->id = id;
    buf_dsc->img_dsc.header.w = w;
    buf_dsc->img_dsc.header.h = h;
    buf_dsc->img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    buf_dsc->img_dsc.data_size = size;
    buf_dsc->img_dsc.data = (const uint8_t*)data;
}

// for lvgl mainwnd
static bool acquireDrawBuffer(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc) {
    ASSERT_NOT_NULLPTR(buf_dsc);

    WindowNode* node = toWindowNode(meta);
    ASSERT_NOT_NULLPTR(node);

    BufferItem* bufItem = node->acquireBuffer();
    if (bufItem == nullptr) {
        ALOGI("acquire buffer from state failure!");
        return false;
    }

    Rect& rect = node->getRect();

    initBufDsc(buf_dsc, bufItem->mKey, rect.getWidth(), rect.getHeight(), bufItem->mSize,
               bufItem->mBuffer);
    return true;
}

bool releaseDrawBuffer(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc) {
    WindowNode* node = toWindowNode(meta);
    ASSERT_NOT_NULLPTR(node);

    if (buf_dsc) {
        node->releaseBuffer();
    }
    return false;
}

static bool sendInputEvent(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_input_event_t* event) {
    if (event == nullptr) {
        ALOGW("input event: shouldn't send null event!");
        return false;
    }
    WindowNode* node = toWindowNode(meta);
    if (node == nullptr) {
        ALOGW("input event: no valid window, cann't send it!");
        return false;
    }

    const InputMessage* ie = (const InputMessage*)event;
    return node->getState()->sendInputMessage(ie);
}

static inline void setWidgetMetaInfo(lv_obj_t* obj, void* data, bool enableInput) {
    lv_mainwnd_metainfo_t meta;
    meta.acquire_buffer = acquireDrawBuffer;
    meta.release_buffer = releaseDrawBuffer;
    if (enableInput) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        meta.send_input_event = sendInputEvent;
    } else {
        meta.send_input_event = nullptr;
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    meta.on_destroy = nullptr;
    meta.info = data;
    lv_mainwnd_set_metainfo(obj, &meta);
}

WindowNode::WindowNode(WindowState* state, lv_obj_t* parent, const Rect& rect, bool enableInput)
      : mState(state), mBuffer(nullptr), mRect(rect) {
    mWidget = lv_mainwnd_create(parent);

    // init window position and size
    lv_obj_set_pos(mWidget, mRect.left, mRect.top);
    lv_obj_set_size(mWidget, mRect.right, mRect.bottom);

    // init window meta information
    setWidgetMetaInfo(mWidget, this, enableInput);
}

WindowNode::~WindowNode() {
    if (mWidget) lv_obj_del(mWidget);
    releaseBuffer();
    mState.reset();
}

bool WindowNode::updateBuffer(BufferItem* item, Rect* rect) {
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
        initBufDsc(&dsc, mBuffer->mKey, mRect.getWidth(), mRect.getHeight(), mBuffer->mSize,
                   mBuffer->mBuffer);
        result = lv_mainwnd_update_buffer(mWidget, &dsc, rect ? &area : nullptr);
    } else {
        result = lv_mainwnd_update_buffer(mWidget, NULL, NULL);
    }

    ALOGI("WMS updateBuffer(%s) from(%d) to(%d)\n", result ? "success" : "failure",
          oldBuffer ? oldBuffer->mKey : -1, mBuffer ? mBuffer->mKey : -1);

    // need to reset buffer
    if (!result) {
        mBuffer = oldBuffer;
    } else if (oldBuffer && !mState->releaseBuffer(oldBuffer)) {
        ALOGE("WMS releaseBuffer(%d) exception\n", oldBuffer->mKey);
        return false;
    }
    return result;
}

BufferItem* WindowNode::acquireBuffer() {
    if (!mBuffer) {
        mBuffer = mState->acquireBuffer();
    }
    return mBuffer;
}

bool WindowNode::releaseBuffer() {
    if (mBuffer) {
        return mState->releaseBuffer(mBuffer);
    }
    return false;
}

void WindowNode::enableInput(bool enable) {
    setWidgetMetaInfo(mWidget, this, enable);
}

} // namespace wm

} // namespace os
