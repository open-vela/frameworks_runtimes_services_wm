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
    buf_dsc->w = w;
    buf_dsc->h = h;
    buf_dsc->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    buf_dsc->data_size = size;
    buf_dsc->data = data;
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

static int sendInputEvent(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_input_event_t* event) {
    if (event == nullptr) {
        return 1;
    }
    WindowNode* node = toWindowNode(meta);
    ASSERT_NOT_NULLPTR(node);

    // InputMessage ie;
    // //TODO: translate mainwnd input event
    // return node->sendInputMessage(&ie);
    return 0;
}

WindowNode::WindowNode(WindowState* state, lv_obj_t* parent, const Rect& rect)
      : mState(state), mBuffer(nullptr), mRect(rect) {
    mWidget = lv_mainwnd_create(parent);

    // init window position and size
    lv_obj_set_pos(mWidget, mRect.left, mRect.top);
    lv_obj_set_size(mWidget, mRect.right, mRect.bottom);

    // init window meta information
    lv_mainwnd_metainfo_t meta;
    meta.acquire_buffer = acquireDrawBuffer;
    meta.release_buffer = releaseDrawBuffer;
    meta.send_input_event = sendInputEvent;
    meta.on_destroy = nullptr;
    meta.info = this;
    lv_mainwnd_set_metainfo(mWidget, &meta);
}

WindowNode::~WindowNode() {
    if (mWidget) lv_obj_del(mWidget);
    releaseBuffer();
    mState.reset();
}

bool WindowNode::updateBuffer(BufferItem* item, Rect* rect) {
    lv_mainwnd_buf_dsc_t dsc;
    lv_area_t area;

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
        lv_mainwnd_update_buffer(mWidget, &dsc, rect ? &area : nullptr);
    } else {
        lv_mainwnd_update_buffer(mWidget, NULL, NULL);
    }
    return true;
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

} // namespace wm
} // namespace os
