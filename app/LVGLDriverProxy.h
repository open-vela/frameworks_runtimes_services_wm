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
#ifdef CONFIG_LVGL_EXTENSION
#include <ext/lv_ext.h>
#endif

#include <vector>

#include "UIDriverProxy.h"

namespace os {
namespace wm {

#define UI_PROXY_TIMER_PERIOD LV_DEF_REFR_PERIOD
#define UI_PROXY_TIMER_READY LV_NO_TIMER_READY

class LVGLDrawBuffer {
public:
    LVGLDrawBuffer(void* rawBuffer, uint32_t width, uint32_t height, lv_color_format_t cf,
                   uint32_t size);
    ~LVGLDrawBuffer();

    lv_draw_buf_t* getDrawBuffer() {
        return &mDrawBuffer;
    }

private:
    lv_draw_buf_t mDrawBuffer;
};

class LVGLDriverProxy : public UIDriverProxy {
public:
    LVGLDriverProxy(std::shared_ptr<BaseWindow> win);
    ~LVGLDriverProxy();

    void* getRoot() override;
    void* getWindow() override;
    void drawFrame(BufferItem* bufItem) override;

    void updateResolution(int32_t width, int32_t height, uint32_t format) override;
    void handleEvent() override;
    void setInputMonitor(InputMonitor* monitor) override;

    lv_display_render_mode_t renderMode() {
        return mRenderMode;
    }
    void updateVisibility(bool visible);

    virtual void* onDequeueBuffer() override;
    virtual void resetBuffer() override;

    static void init();
    static void deinit();
    static void vsyncPollEvent(vector<std::shared_ptr<BaseWindow>> listeners);
    static inline uint32_t getTimerPeriod() {
        return LV_DEF_REFR_PERIOD;
    }

    static inline uint32_t timerHandler() {
        return lv_timer_handler();
    }

    static inline void setTimerResumeHandler(lv_timer_handler_resume_cb_t cb, void* data) {
        lv_timer_handler_set_resume_cb(cb, data);
    }

    lv_display_t* mDisp;
    int32_t mDispW;
    int32_t mDispH;
    lv_indev_t* mIndev;
    lv_indev_state_t mLastEventState;
    lv_display_render_mode_t mRenderMode;
    lv_draw_buf_t* mDummyBuffer;
    ::std::vector<std::shared_ptr<LVGLDrawBuffer>> mDrawBuffers;
};

} // namespace wm
} // namespace os
