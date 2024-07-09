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

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
#include <animengine/anim_api.h>
#endif

#include <nuttx/config.h>

#ifdef CONFIG_ANIMATION_ENGINE
typedef void* AnimEngineHandle;
#endif

namespace os {
namespace wm {

enum WindowAnimType {
    WINDOW_ANIM_TYPE_ALPHA = 1,
    WINDOW_ANIM_TYPE_SLIDE,
    WINDOW_ANIM_TYPE_SCALE,
    WINDOW_ANIM_TYPE_ROTATE,
};

enum WindowAnimStatus {
    WINDOW_ANIM_STATUS_STARTING = 1,
    WINDOW_ANIM_STATUS_FINISHED,
};

} // namespace wm
} // namespace os