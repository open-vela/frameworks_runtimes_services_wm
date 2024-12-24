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

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION

/**
 * @namespace os::wm
 * @brief The namespace for window management related classes and functions.
 */
namespace os {
namespace wm {

/**
 * @class WindowAnimEngine
 * @brief Class for managing window transition animations.
 *
 * This class encapsulates the functionality required to create and manage
 * animations for window transitions, allowing for smooth visual effects
 * during window state changes.
 */
class WindowAnimEngine {
public:
    WindowAnimEngine();

    ~WindowAnimEngine();

    /**
     * @brief Retrieves the handle to the underlying animation engine.
     *
     * This method provides access to the AnimEngineHandle used for
     * performing animations.
     *
     * @return Handle to the animation engine.
     */
    AnimEngineHandle getEngine();

private:
    AnimEngineHandle mAnimEngine;
};

} // namespace wm
} // namespace os

#endif
