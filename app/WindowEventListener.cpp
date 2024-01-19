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

#include "wm/WindowEventListener.h"

#include "../common/WindowUtils.h"

namespace os {
namespace wm {

WindowEventListener::WindowEventListener(void* data) : mData(data) {}

WindowEventListener::~WindowEventListener() {}

void WindowEventListener::onSizeChanged(uint32_t w, uint32_t h, uint32_t oldw, uint32_t oldh) {
    FLOGI(" %" PRId32 "x%" PRId32 " from %" PRId32 "x%" PRId32 " ", w, h, oldw, oldh);
}

/*for mock ui*/
void WindowEventListener::onTouch(int32_t x, int32_t y) {}
void WindowEventListener::onDraw(void* buffer, uint32_t size) {}
void WindowEventListener::onPostDraw() {}

} // namespace wm
} // namespace os
