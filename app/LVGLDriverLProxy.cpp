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

#include <lv_porting/lv_porting.h>
#include <lvgl/lvgl.h>

#include "LVGLDriverProxy.h"

namespace os {
namespace wm {

LVGLDriverProxy::LVGLDriverProxy(std::shared_ptr<BaseWindow> win)
      : UIDriverProxy(win), mDisp(nullptr), mIndev(nullptr) {
    initUIInstance();
}

LVGLDriverProxy::~LVGLDriverProxy() {
    // TODO: lv_deinit
}

bool LVGLDriverProxy::initUIInstance() {
    lv_init();

    // TODO: custom init for porting
    lv_porting_init();

    mDisp = lv_disp_get_default();
    if (mDisp) {
        // for v9
        // lv_disp_set_user_data(mDisp, this);
        return true;
    }
    return false;
}

void LVGLDriverProxy::drawFrame(BufferItem* bufItem) {
    BufferItem* oldItem = getBufferItem();
    if (oldItem) {
        // TODO: copy old item buffer to current buffer
    }

    UIDriverProxy::drawFrame(bufItem);

    // need to custom LVGL display
    lv_refr_now(mDisp);
}

void LVGLDriverProxy::handleEvent(InputMessage& message) {
    // TODO: switch message to indev event
}

void* LVGLDriverProxy::getRoot() {
    return mDisp;
}

void* LVGLDriverProxy::getWindow() {
    return lv_disp_get_scr_act(mDisp);
}

} // namespace wm
} // namespace os
