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

#ifdef LV_DEF_REFR_PERIOD
#define DEF_REFR_PERIOD LV_DEF_REFR_PERIOD
#else
#define DEF_REFR_PERIOD 16
#endif
namespace os {
namespace wm {
class WindowManagerService;

typedef enum {
    DSM_TIMER = 0,
    DSM_VSYNC = 1,
} DispSyncMode;

class RootContainer {
public:
    RootContainer(WindowManagerService* service);
    ~RootContainer();

    lv_disp_t* getRoot();
    lv_obj_t* getDefLayer();
    lv_obj_t* getSysLayer();
    lv_obj_t* getTopLayer();

    bool getDisplayInfo(DisplayInfo* info);

    void enableVsync(bool enable);
    void processVsyncEvent();

    void showToast(const char* text, uint32_t duration);

private:
    bool init();
    WindowManagerService* mService;
    lv_disp_t* mDisp;
    lv_indev_t* mIndev;
    lv_timer_t* mVsyncTimer;
    void* mUvData;
};

} // namespace wm
} // namespace os
