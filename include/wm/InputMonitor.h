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

#include <uv.h>
#include <wm/InputChannel.h>
#include <wm/InputMessage.h>

/**
 * @namespace os::wm
 * @brief The namespace for the window manager input handling classes.
 */
namespace os {
namespace wm {

class InputMonitor;

typedef std::function<void(InputMonitor*)> InputMonitorCallback;

/**
 * @class InputMonitor
 * @brief Class for monitoring input events from an InputChannel.
 *
 * This class facilitates the detection and handling of input events
 * from a specified input channel. It provides functionality to start
 * and stop monitoring events, as well as to process the received
 * input messages.
 */
class InputMonitor {
public:
    InputMonitor();

    InputMonitor(const sp<IBinder> token, InputChannel* channel);

    ~InputMonitor();

    /**
     * @brief Sets the input channel for the monitor.
     *
     * @param inputChannel Pointer to the InputChannel to be monitored.
     */
    void setInputChannel(InputChannel* inputChannel);

    /**
     * @brief Checks if the input monitor is valid.
     *
     * @return True if the input channel is valid, false otherwise.
     */
    bool isValid() {
        return mInputChannel && mInputChannel.get() ? mInputChannel->isValid() : false;
    }

    /**
     * @brief Receives an input message and processes it.
     *
     * This method handles the input message received from the input channel.
     *
     * @param msg Pointer to the InputMessage containing the event data.
     * @return True if the message is successfully processed, false otherwise.
     */
    bool receiveMessage(const InputMessage* msg);
    bool empty();

    /**
     * @brief Starts monitoring input events.
     *
     * This method initiates the monitoring process using a specified event loop.
     *
     * @param loop Pointer to the UV loop for handling events.
     * @param callback Function to be called for input monitoring events.
     * @return True if monitoring starts successfully, false otherwise.
     */
    bool start(uv_loop_t* loop, InputMonitorCallback callback);

    /**
     * @brief Retrieves the associated input channel.
     *
     * @return Shared pointer to the associated InputChannel.
     */
    std::shared_ptr<InputChannel>& getInputChannel() {
        return mInputChannel;
    }

    /**
     * @brief Retrieves the binder token associated with the input monitor.
     *
     * @return Reference to the binder token.
     */
    sp<IBinder>& getToken() {
        return mToken;
    }

    /**
     * @brief Prevents copy and assignment by deleting the copy constructor and assignment operator.
     */
    DISALLOW_COPY_AND_ASSIGN(InputMonitor);

private:
    void stop();

    sp<IBinder> mToken;
    std::shared_ptr<InputChannel> mInputChannel;
    uv_poll_t* mPoll;
    InputMonitorCallback mEventHandler;
};

} // namespace wm
} // namespace os
