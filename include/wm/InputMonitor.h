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

typedef std::function<void(InputMonitor*)> InputMonitorCallback;

class InputMonitor {
public:
    InputMonitor();
    InputMonitor(const sp<IBinder> token, InputChannel* channel);
    ~InputMonitor();

    void setInputChannel(InputChannel* inputChannel);

    bool isValid() {
        return mInputChannel && mInputChannel.get() ? mInputChannel->isValid() : false;
    }

    bool receiveMessage(const InputMessage* msg);

    bool start(uv_loop_t* loop, InputMonitorCallback callback);

    std::shared_ptr<InputChannel>& getInputChannel() {
        return mInputChannel;
    }
    sp<IBinder>& getToken() {
        return mToken;
    }

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
