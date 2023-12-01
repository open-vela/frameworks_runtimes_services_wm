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

namespace os {
namespace wm {

class InputMonitor;

class EventHandler {
public:
    virtual void handleEvent() = 0;
};

class DefaultEventHandler : public EventHandler {
public:
    DefaultEventHandler(InputMonitor* monitor) : mInputMonitor(monitor) {}

    void handleEvent() override;

private:
    InputMonitor* mInputMonitor;
};

class InputMonitor {
public:
    InputMonitor();
    InputMonitor(const sp<IBinder> token);
    ~InputMonitor();

    void setInputChannel(InputChannel* inputChannel);

    bool isValid() {
        return mInputChannel != nullptr ? mInputChannel->isValid() : false;
    }

    bool receiveMessage(const InputMessage* msg);

    void start(uv_loop_t* loop, EventHandler* handler);

    DISALLOW_COPY_AND_ASSIGN(InputMonitor);

private:
    void stop();

    sp<IBinder> mToken;
    InputChannel* mInputChannel;
    uv_poll_t* mPoll;
};

} // namespace wm
} // namespace os
