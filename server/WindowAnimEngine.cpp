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

#define LOG_TAG "WindowAnimEngine"

#include "WindowAnimEngine.h"

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION

#include "ext/animengine/lvx_animengine_adapter.h"

namespace os {
namespace wm {

WindowAnimEngine::WindowAnimEngine() {
    mAnimEngine = lvx_anim_adapter_init();
}

WindowAnimEngine::~WindowAnimEngine() {
    lvx_anim_adapter_uninit(mAnimEngine);
}

AnimEngineHandle WindowAnimEngine::getEngine() {
    return mAnimEngine;
}

} // namespace wm
} // namespace os

#endif
