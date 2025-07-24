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

namespace os {
namespace wm {

/**
 * @brief DummyDriverProxy is a proxy class for the UI driver.
 *
 * This class serves as a mock implementation of the UIDriverProxy,
 * allowing for testing and simulation of driver behavior in a safe
 * environment without relying on actual graphics hardware.
 */
class DummyDriverProxy : public UIDriverProxy {
public:
    DummyDriverProxy(std::shared_ptr<BaseWindowDefault> win);

    ~DummyDriverProxy();

    /**
     * @brief Retrieves the root object.
     *
     * This method overrides the base class method to return the root
     * element associated with this driver proxy.
     *
     * @return A pointer to the root object.
     */
    void* getRoot() override;

    /**
     * @brief Retrieves the window object.
     *
     * This method overrides the base class method to return the window
     * associated with this driver proxy.
     *
     * @return A pointer to the window object.
     */
    void* getWindow() override;

    /**
     * @brief Handles input events directed to the UI.
     *
     * This method overrides the base class method to implement event
     * handling logic for the dummy driver.
     */
    void handleEvent() override;

    /**
     * @brief Draws a frame with the provided buffer item.
     *
     * This method overrides the base class method to simulate frame
     * drawing using the given buffer item.
     *
     * @param bufItem A pointer to the BufferItem to be drawn.
     */
    void drawFrame(BufferItem* bufItem) override;

private:
    bool mActive;
};

} // namespace wm
} // namespace os
