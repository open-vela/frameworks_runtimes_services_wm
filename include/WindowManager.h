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

/**
 * @file WindowManager.h
 * @brief Window Manager Header
 *
 * This file defines the Window Manager class, responsible for managing the creation, layout, and
 * removal of application windows.
 */

#include <pthread.h>

#include <mutex>

#include "BaseWindow.h"
#include "app/Context.h"
#include "os/wm/IWindowManager.h"
#include "wm/InputMonitor.h"

namespace os {
namespace wm {

using android::sp;

class BaseWindow;
class SurfaceTransaction;

/**
 * @class WindowManager
 * @brief Window Manager Class
 *
 * This class is responsible for managing the creation, layout, and removal of application windows.
 */
class WindowManager {
public:
    WindowManager() = default;
    virtual ~WindowManager() = default;

    static WindowManager* create();

    static inline const char* name() {
        return "window";
    }

    /**
     * @brief Monitor input
     * @param name The name of the monitor
     * @param displayId The display ID
     * @return A shared pointer to the input monitor
     */
    static std::shared_ptr<InputMonitor> monitorInput(const ::std::string& name, int32_t displayId);

    /**
     * @brief Release input monitor
     * @param monitor The input monitor to release
     */
    static void releaseInput(InputMonitor* monitor);

    /**
     * @brief Create a new window
     * @param context The application context
     * @return A shared pointer to the newly created window
     */
    virtual std::shared_ptr<BaseWindow> newWindow(::os::app::Context* context) = 0;

    /**
     * @brief Attach a window
     * @param window The smart pointer to the window to attach
     * @return The ID of the attached window
     */
    virtual int32_t attachIWindow(std::shared_ptr<BaseWindow> window) = 0;

    /**
     * @brief Relayout a window
     * @param window The smart pointer to the window to relayout
     */
    virtual void relayoutWindow(std::shared_ptr<BaseWindow> window) = 0;

    /**
     * @brief Remove a window
     * @param window The smart pointer to the window to remove
     * @return True if the window was successfully removed, false otherwise
     */
    virtual bool removeWindow(std::shared_ptr<BaseWindow> window) = 0;

    /**
     * @brief Print information about all windows
     * @return True if successful, false otherwise
     */
    virtual bool dumpWindows() = 0;

    /**
     * @brief Get display information
     * @param width Pointer to the display width
     * @param height Pointer to the display height
     */
    virtual void getDisplayInfo(uint32_t* width, uint32_t* height) const = 0;

    /**
     * @brief Get the window manager service
     * @return A reference to the window manager service's smart pointer
     */
    virtual sp<IWindowManager>& getService() = 0;
};

} // namespace wm
} // namespace os
