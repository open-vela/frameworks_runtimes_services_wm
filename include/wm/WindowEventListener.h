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

#include <stdio.h>

namespace os {
namespace wm {

class WindowEventListener {
public:
    WindowEventListener(void* data);
    virtual ~WindowEventListener();
    virtual void onSizeChanged(uint32_t w, uint32_t h, uint32_t oldw, uint32_t olh);

    virtual void onTouch(int32_t x, int32_t y);
    virtual void onDraw(void* buffer, uint32_t size);
    virtual void onPostDraw();
    void* getData() {
        return mData;
    }

private:
    void* mData;
};

} // namespace wm
} // namespace os
