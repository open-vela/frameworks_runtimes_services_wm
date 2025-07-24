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

#include <android-base/macros.h>
#include <utils/RefBase.h>

#include <atomic>

#include "WindowManager.h"
#include "app/Context.h"
#include "app/UvLoop.h"
#include "wm/InputMonitor.h"
#include "wm/LayoutParams.h"
#include "wm/WindowEventListener.h"

namespace os {
namespace wm {

class WindowManager;

using android::sp;

/**
 * @class BaseWindow
 * @brief Represents a window in the window manager.
 *
 * This class is responsible for handling window-related operations such as
 * movement, resizing, visibility, and event handling.
 */
class BaseWindow : public std::enable_shared_from_this<BaseWindow> {
public:
    BaseWindow(){};
    virtual ~BaseWindow() = default;

    DISALLOW_COPY_AND_ASSIGN(BaseWindow);

    /**
     * @brief Creates the stage for the window.
     *
     * @return True if the stage was created successfully, false otherwise.
     */
    virtual bool onCreateStage() = 0;

    /**
     * @brief Destroys the stage for the window.
     *
     * @return True if the stage was destroyed successfully, false otherwise.
     */
    virtual bool onDestroyStage() = 0;

    /**
     * @brief Retrieves the root object.
     *
     * @return Pointer to the root object.
     */
    virtual void* getRoot() = 0;

    /**
     * @brief Sets the type of the window.
     *
     * @param type The type of the window.
     */
    virtual void setType(int32_t type) = 0;

    /**
     * @brief Sets the visibility state of the window.
     *
     * @param visible True to make the window visible, false to hide it.
     */
    virtual void setVisible(bool visible) = 0;

    /**
     * @brief Sets the layout parameters for the window.
     *
     * @param lp The layout parameters for the window.
     */
    virtual void setLayoutParams(LayoutParams lp) = 0;

    /**
     * @brief Retrieves the layout parameters of the window.
     *
     * @return The layout parameters.
     */
    virtual LayoutParams getLayoutParams() = 0;

    /**
     * @brief Retrieves the associated WindowManager.
     *
     * @return Pointer to the WindowManager.
     */
    virtual const WindowManager* getWindowManager() = 0;

    /**
     * @brief Retrieves the visibility state of the window.
     *
     * @return The visibility state.
     */
    virtual int32_t getVisibility() = 0;

    /**
     * @brief Retrieves the application context.
     * @return Pointer to the application context.
     */
    virtual ::os::app::Context* getContext() = 0;

    /**
     * @brief Retrieves the native display object.
     *
     * @return Pointer to the native display object.
     */
    virtual void* getNativeDisplay() = 0;

    /**
     * @brief Sets an event listener for window events.
     *
     * @param listener Pointer to the WindowEventListener.
     */
    virtual void setEventListener(WindowEventListener* listener) = 0;

    /**
     * @brief Traces the frame if enabled.
     *
     * @param enable True to enable frame tracing, false to disable.
     */
    virtual void traceFrame(bool enable) = 0;
};

} // namespace wm
} // namespace os
