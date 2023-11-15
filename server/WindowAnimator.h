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

#include "WindowConfig.h"
#include "wm/Rect.h"

#ifdef CONFIG_ANIMATION_ENGINE
#include <anim_api.h>
#endif
#include <fstream>
#include <functional>
#include <iostream>

#include "lvgl/lv_mainwnd.h"

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
namespace os {
namespace wm {
typedef std::function<void(WindowAnimStatus status)> AnimCallback;

class WindowAnimator {
public:
    WindowAnimator(AnimEngineHandle animEngine, lv_obj_t* widget);
    ~WindowAnimator();

    int startAnimation(std::string animConfig, AnimCallback callback);
    void cancelAnimation();

    AnimCallback mAnimStatusCB;
    WindowAnimStatus mAnimStatus;

private:
    AnimEngineHandle mAnimEngine;
    int64_t mAnimId;
    lv_obj_t* mWidget;
};

} // namespace wm
} // namespace os

#endif