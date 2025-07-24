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

#include <lvgl/lvgl.h>
#include <os/wm/DisplayInfo.h>
#include <uv.h>

#include "../common/FrameTimeInfo.h"
#include "DeviceEventListener.h"

namespace os {
namespace wm {

/**
 * @class RootContainer
 * @brief Manages the root display and event handling for the UI.
 *
 * The RootContainer class is responsible for managing the main display
 * and handling events related to user interaction and rendering. It
 * oversees various UI layers and controls the rendering process.
 */
class RootContainer {
public:
    RootContainer(DeviceEventListener* listener, uv_loop_t* loop);

    ~RootContainer();

    /**
     * @brief Retrieves the root display object.
     *
     * @return Pointer to the LVGL display object representing the root.
     */
    lv_disp_t* getRoot();

    /**
     * @brief Retrieves the default layer for the UI.
     *
     * @return Pointer to the LVGL object representing the default layer.
     */
    lv_obj_t* getDefLayer();

    /**
     * @brief Retrieves the system layer for the UI.
     *
     * @return Pointer to the LVGL object representing the system layer.
     */
    lv_obj_t* getSysLayer();

    /**
     * @brief Retrieves the topmost layer for rendering.
     *
     * @return Pointer to the LVGL object representing the top layer.
     */
    lv_obj_t* getTopLayer();

    /**
     * @brief Retrieves display information for the root container.
     *
     * @param info Pointer to a DisplayInfo structure to store retrieved info.
     * @return True if the information is successfully retrieved, false otherwise.
     */
    bool getDisplayInfo(DisplayInfo* info);

    /**
     * @brief Displays a toast message on the screen.
     *
     * @param text The message text to display.
     * @param duration How long the toast should be visible (in milliseconds).
     */
    void showToast(const char* text, uint32_t duration);

    /**
     * @brief Reads input events from the specified input device.
     *
     * @param drv Pointer to the LVGL input device to read from.
     * @param data Pointer to the LVGL input data structure to fill.
     * @return True if the input is read successfully, false otherwise.
     */
    bool readInput(lv_indev_t* drv, lv_indev_data_t* data);

    /**
     * @brief Checks if the root container is ready for use.
     *
     * @return True if the root container is ready, false otherwise.
     */
    bool ready() {
        return mReady;
    }

    /**
     * @brief Retrieves the current frame meta information.
     *
     * @return Pointer to the FrameMetaInfo associated with the root container.
     */
    FrameMetaInfo* frameInfo();

    /**
     * @brief Called at the start of a frame's processing.
     */
    void onFrameStart();

    /**
     * @brief Called at the start of rendering a frame.
     */
    void onRenderStart();

    /**
     * @brief Called when a frame has finished being processed.
     */
    void onFrameFinished();

    /**
     * @brief Enables or disables frame tracing.
     *
     * @param enable True to enable tracing, false to disable.
     */
    void traceFrame(bool enable);

    /**
     * @brief Enables or disables Vsync for the root container for multi-instance mode.
     *
     * @param enable True to enable Vsync, false to disable it.
     */
    void enableVsync(bool enable);

    /**
     * @brief Processes Vsync events for multi-instance mode.
     *
     * This method handles the processing of incoming Vsync events
     * for the root container, updating the UI as necessary.
     */
    void processVsyncEvent();

    /**
     * @brief Checks if Vsync is currently enabled for multi-instance mode.
     *
     * @return True if Vsync is enabled, false otherwise.
     */
    bool vsyncEnabled() {
        return mVsyncEnabled;
    }

private:
    bool init();

    lv_nuttx_result_t mResult;
    DeviceEventListener* mListener;
    lv_disp_t* mDisp;
    void* mUvData;
    uv_loop_t* mUvLoop;
    bool mReady;

    bool mVsyncEnabled;
    lv_timer_t* mVsyncTimer;

    bool mTraceFrame;
    FrameMetaInfo mFrameInfo;
    FrameTimeInfo mFrameTimeInfo;
};

} // namespace wm
} // namespace os
