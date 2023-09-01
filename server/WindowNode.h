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

#pragma once

#include "WindowState.h"
#include "lvgl/lv_mainwnd.h"
#include "wm/InputMessage.h"
#include "wm/Rect.h"

namespace os {
namespace wm {

class WindowNode {
public:
    WindowNode(WindowState* state, lv_obj_t* parent, const Rect& rect, bool enableInput);
    ~WindowNode();

    bool updateBuffer(BufferItem* item, Rect* rect);

    BufferItem* acquireBuffer();
    bool releaseBuffer();

    Rect& getRect() {
        return mRect;
    }

    WindowState* getState() {
        return mState;
    }

    void enableInput(bool enable);

    DISALLOW_COPY_AND_ASSIGN(WindowNode);

private:
    WindowState* mState;
    BufferItem* mBuffer;
    lv_obj_t* mWidget;
    Rect mRect;
};

} // namespace wm
} // namespace os
