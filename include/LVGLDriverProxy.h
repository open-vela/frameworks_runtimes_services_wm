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

#include "UIDriverProxy.h"
#include "lvgl/lvgl.h"

namespace os {
namespace wm {

class LVGLDriverProxy : public UIDriverProxy {
public:
    LVGLDriverProxy(std::shared_ptr<BaseWindow> win);
    ~LVGLDriverProxy();

    bool initUIInstance() override;

    // to window: ui proxy send event, and window need process it
    void onInvalidate() override;
    void* onDequeueBuffer() override;
    int onQueueBuffer() override;
    void* onCancelBuffer() override;

    // to proxy: window request proxy to render ui
    bool drawFrame(std::shared_ptr<BufferItem> bufItem) override;
    void* getRoot() override;

private:
    lv_disp_t* mDisp;
    lv_indev_t* mIndev;
};

} // namespace wm
} // namespace os