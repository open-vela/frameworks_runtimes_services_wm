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

#include "DeviceEventListener.h"

namespace os {
namespace wm {

class RootContainer {
public:
    RootContainer(DeviceEventListener* listener, uv_loop_t* loop);
    ~RootContainer();

    lv_disp_t* getRoot();
    lv_obj_t* getDefLayer();
    lv_obj_t* getSysLayer();
    lv_obj_t* getTopLayer();

    bool getDisplayInfo(DisplayInfo* info);

    void enableVsync(bool enable);
    void processVsyncEvent();

    void showToast(const char* text, uint32_t duration);

    bool readInput(lv_indev_t* drv, lv_indev_data_t* data);

    bool vsyncEnabled() {
        return mVsyncEnabled;
    }
    bool ready() {
        return mReady;
    }

private:
    bool init();
    lv_nuttx_result_t mResult;

    DeviceEventListener* mListener;
    lv_disp_t* mDisp;
    bool mVsyncEnabled;
#ifndef CONFIG_SYSTEM_WINDOW_USE_VSYNC_EVENT
    lv_timer_t* mVsyncTimer;
#endif
    void* mUvData;
    uv_loop_t* mUvLoop;
    bool mReady;
};

} // namespace wm
} // namespace os
