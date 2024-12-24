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

#include <wm/InputChannel.h>
#include <wm/InputMessage.h>

/**
 * @namespace os::wm
 * @brief The namespace for window management related classes and functionalities.
 */
namespace os {
namespace wm {

// Forward declarations for Android namespaces used.
using namespace android;
using namespace android::base;
using namespace android::binder;
using namespace std;

/**
 * @class InputDispatcher
 * @brief Manages the dispatching of input messages to the corresponding input channels.
 *
 * The InputDispatcher class is responsible for handling input events by sending
 * messages to the appropriate input channels. It facilitates communication between
 * the input system and window management, ensuring events are processed correctly.
 */
class InputDispatcher {
public:
    InputDispatcher();

    ~InputDispatcher();

    /**
     * @brief Releases the resources associated with the InputDispatcher.
     *
     * This method will clean up the resources held by the dispatcher.
     */
    void release();

    /**
     * @brief Sends a message to the designated input channel.
     *
     * This method dispatches the specified input message to the input system.
     *
     * @param ie Pointer to the InputMessage to be sent.
     * @return An integer representing the status of the send operation.
     */
    int sendMessage(const InputMessage* ie);

    /**
     * @brief Retrieves the input channel managed by this dispatcher.
     *
     * @return A reference to the associated InputChannel object.
     */
    InputChannel& getInputChannel() {
        return mInputChannel;
    }

    /**
     * @brief Creates a new InputDispatcher instance.
     *
     * This static method initializes a new instance of InputDispatcher with the given name.
     *
     * @param name The name to associate with the new InputDispatcher instance.
     * @return A shared pointer to the newly created InputDispatcher.
     */
    static std::shared_ptr<InputDispatcher> create(const std::string& name);

    /**
     * @brief Disallows copy and assignment of InputDispatcher.
     */
    DISALLOW_COPY_AND_ASSIGN(InputDispatcher);

private:
    InputChannel mInputChannel;
    int mErrCount;
};

} // namespace wm
} // namespace os
