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
#include <uikit/uikit.h>

#include <vector>

#include "UIDriverProxy.h"

namespace os {
namespace wm {

/**
 * @def UI_PROXY_TIMER_PERIOD
 * @brief Defines the timer period for the UI proxy.
 */
#define UI_PROXY_TIMER_PERIOD LV_DEF_REFR_PERIOD

/**
 * @def UI_PROXY_TIMER_READY
 * @brief Defines the timer ready state for the UI proxy.
 */
#define UI_PROXY_TIMER_READY LV_NO_TIMER_READY

/**
 * @brief LVGLDrawBuffer is a wrapper class for the buffer used in LVGL drawing.
 *
 * This class manages the internal drawing buffer used for rendering graphics
 * in the LVGL context.
 */
class LVGLDrawBuffer {
public:
    LVGLDrawBuffer(void* rawBuffer, uint32_t width, uint32_t height, lv_color_format_t cf,
                   uint32_t size);

    ~LVGLDrawBuffer();

    /**
     * @brief Retrieves the LVGL draw buffer structure.
     *
     * @return A pointer to the lv_draw_buf_t structure.
     */
    lv_draw_buf_t* getDrawBuffer() {
        return &mDrawBuffer;
    }

private:
    lv_draw_buf_t mDrawBuffer;
};

/**
 * @brief LVGLDriverProxy is an implementation of the UIDriverProxy for LVGL.
 *
 * This class manages the UI rendering using the LVGL framework and
 * handles input events specific to LVGL interfaces.
 */
class LVGLDriverProxy : public UIDriverProxy {
public:
    LVGLDriverProxy(std::shared_ptr<BaseWindowDefault> win);

    ~LVGLDriverProxy();

    /**
     * @brief Retrieves the root object for the LVGL driver.
     *
     * This method overrides the base class method to provide the root
     * element associated with this LVGL driver proxy.
     *
     * @return A pointer to the root object.
     */
    void* getRoot() override;

    /**
     * @brief Retrieves the window object for the LVGL driver.
     *
     * This method overrides the base class method to provide the window
     * associated with this LVGL driver proxy.
     *
     * @return A pointer to the window object.
     */
    void* getWindow() override;

    /**
     * @brief Draws a frame using the provided buffer item.
     *
     * This method overrides the base class method to implement frame
     * drawing logic using the LVGL framework.
     *
     * @param bufItem A pointer to the BufferItem to be drawn.
     */
    void drawFrame(BufferItem* bufItem) override;

    /**
     * @brief Updates the resolution of the display.
     *
     * @param width New width of the display.
     * @param height New height of the display.
     * @param format New color format of the display.
     */
    void updateResolution(int32_t width, int32_t height, uint32_t format) override;

    /**
     * @brief Handles input events for the LVGL driver.
     *
     * This method overrides the base class method to provide event
     * handling specific to LVGL input.
     */
    void handleEvent() override;

    /**
     * @brief Sets the input monitor for the LVGL driver.
     *
     * @param monitor Pointer to the InputMonitor instance.
     */
    void setInputMonitor(InputMonitor* monitor) override;

    /**
     * @brief Updates the visibility state of the LVGL driver.
     *
     * @param visible New visibility state.
     */
    void updateVisibility(bool visible) override;

    /**
     * @brief On dequeue buffer callback.
     *
     * This method is called when a buffer is dequeued for rendering.
     *
     * @return A pointer to the dequeued buffer.
     */
    void* onDequeueBuffer() override;

    /**
     * @brief Resets the buffer used for rendering.
     */
    void resetBuffer() override;

    /**
     * @brief Updates the dirty area for rendering.
     *
     * @param area Pointer to the area that needs to be updated.
     * @return True if the area is successfully updated, false otherwise.
     */
    bool updateDirtyArea(const lv_area_t* area);

    /**
     * @brief Handles resolution changes.
     *
     * @param width New display width.
     * @param height New display height.
     */
    void onResolutionChanged(int32_t width, int32_t height);

    /**
     * @brief Called when rendering starts.
     */
    void onRenderStart();

    /**
     * @brief Called when rendering ends.
     */
    void onRenderEnd();

    /**
     * @brief Retrieves the current rendering mode.
     *
     * @return The rendering mode.
     */
    int renderMode() {
        return mRenderMode;
    }

    /*
     * @brief Sets the state of the last input event.
     *
     * @param state The state of the last input event.
     */
    void setLastEventState(lv_indev_state_t state) {
        mLastEventState = state;
    }

    /**
     * @brief Retrieves the state of the last input event.
     *
     * @return The state of the last input event.
     */

    lv_indev_state_t getLastEventState() {
        return mLastEventState;
    }

    /**
     * @brief Notifies the driver of a Vsync event.
     *
     * This method overrides the base class method to handle Vsync
     * notifications specific to LVGL.
     */
    void notifyVsyncEvent() override;

    /**
     * @brief Retrieves the timer period for the LVGL driver.
     *
     * @return The timer period in milliseconds.
     */
    uint32_t getTimerPeriod() override {
        return LV_DEF_REFR_PERIOD;
    }

    bool needPeriodicVsync() override;

    /**
     * @brief Initializes the LVGL driver.
     */
    static void init();

    /**
     * @brief Deinitializes the LVGL driver.
     */
    static void deinit();

    /**
     * @brief Timer handler for LVGL.
     *
     * @return The result of the timer handler execution.
     */
    static inline uint32_t timerHandler() {
        return lv_timer_handler();
    }

    /**
     * @brief Sets the timer resume handler for LVGL.
     *
     * @param cb Callback function to resume the timer.
     * @param data User data to be passed to the callback.
     */
    static inline void setTimerResumeHandler(lv_timer_handler_resume_cb_t cb, void* data) {
        lv_timer_handler_set_resume_cb(cb, data);
    }

private:
    lv_display_t* mDisp;

    int32_t mDispW;
    int32_t mDispH;
    lv_indev_t* mIndev;
    int mRenderMode;
    lv_draw_buf_t* mDummyBuffer;
    ::std::vector<std::shared_ptr<LVGLDrawBuffer>> mDrawBuffers;
    bool mAllAreaDirty;
    BufferItem* mPrevBuffer;
    lv_indev_state_t mLastEventState;
};

} // namespace wm
} // namespace os
