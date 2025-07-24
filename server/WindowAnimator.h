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
#include <animengine/anim_api.h>
#endif

#include <fstream>
#include <functional>
#include <iostream>

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION

/**
 * @namespace os::wm
 * @brief The namespace for window management related classes and functionalities.
 */
namespace os {
namespace wm {

/**
 * @brief Defines a callback function type for animation status changes.
 *
 * This type represents a callback function that is invoked to report the
 * status of an animation.
 */
typedef std::function<void(WindowAnimStatus status)> AnimCallback;

/**
 * @class WindowAnimator
 * @brief Class for handling window animations.
 *
 * The WindowAnimator class provides functionality to manage animation
 * effects for window transitions. It interacts with the animation
 * engine to perform smooth visual transitions and manage status
 * updates during the animations.
 */
class WindowAnimator {
public:
    WindowAnimator(AnimEngineHandle animEngine, void* widget);

    ~WindowAnimator();

    /**
     * @brief Starts the animation with the specified configuration.
     *
     * @param animConfig String containing the animation configuration.
     * @param callback Callback function to be called when the animation status changes.
     * @return An integer indicating the success or failure of the animation start request.
     */
    int startAnimation(std::string animConfig, AnimCallback callback);

    /**
     * @brief Cancels any ongoing animation.
     *
     * This method stops the current animation and cleans up resources
     * associated with it.
     */
    void cancelAnimation();

    AnimCallback mAnimStatusCB;
    WindowAnimStatus mAnimStatus;

private:
    AnimEngineHandle mAnimEngine;
    int64_t mAnimId;
    void* mWidget;
};

} // namespace wm
} // namespace os

#endif
