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

#include "wm/InputMessage.h"

/**
 * @namespace os::wm
 * @brief The namespace for window manager related classes and interfaces.
 */
namespace os {
namespace wm {

/**
 * @class DeviceEventListener
 * @brief Interface for handling device events in the window manager.
 *
 * This class defines the methods that should be implemented to handle
 * device events such as Vsync and input messages within the window manager.
 */
class DeviceEventListener {
public:
    /**
     * @brief Pure virtual function for responding to Vsync events.
     *
     * This method should be implemented to define how the event listener
     * responds to Vsync notifications.
     *
     * @return True if the Vsync was successfully handled, false otherwise.
     */
    virtual bool responseVsync() = 0;

    /**
     * @brief Pure virtual function for responding to input messages.
     *
     * This method should be implemented to define how the event listener
     * processes input messages received from the input system.
     *
     * @param msg Pointer to the InputMessage containing event details.
     * @return True if the input message was successfully handled, false otherwise.
     */
    virtual bool responseInput(InputMessage* msg) = 0;
};

} // namespace wm
} // namespace os
