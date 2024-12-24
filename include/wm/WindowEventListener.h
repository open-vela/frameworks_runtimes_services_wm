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

/**
 * @class WindowEventListener
 * @brief Interface for listening to window events.
 *
 * This class provides a mechanism for receiving notifications about
 * various events related to window operations, such as size changes,
 * touch events, and drawing events.
 */
class WindowEventListener {
public:
    WindowEventListener(void* data);

    virtual ~WindowEventListener();

    /**
     * @brief Called when the size of the window changes.
     *
     * @param w New width of the window.
     * @param h New height of the window.
     * @param oldw Previous width of the window.
     * @param oldh Previous height of the window.
     */
    virtual void onSizeChanged(uint32_t w, uint32_t h, uint32_t oldw, uint32_t oldh);

    /**
     * @brief Called when a touch event occurs.
     *
     * @param x X coordinate of the touch event.
     * @param y Y coordinate of the touch event.
     */
    virtual void onTouch(int32_t x, int32_t y);

    /**
     * @brief Called when a draw event occurs.
     *
     * @param buffer Pointer to the drawing buffer.
     * @param size Size of the drawing buffer.
     */
    virtual void onDraw(void* buffer, uint32_t size);

    /**
     * @brief Called after drawing is completed.
     */
    virtual void onPostDraw();

    /**
     * @brief Retrieves the user-defined data associated with the listener.
     *
     * @return Pointer to the associated data.
     */
    void* getData() {
        return mData;
    }

private:
    void* mData;
};

} // namespace wm
} // namespace os
