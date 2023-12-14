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

#define LOG_TAG "InputMonitor"

#include "wm/InputMonitor.h"

#include <mqueue.h>

#include "WindowManager.h"
#include "WindowUtils.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

InputMonitor::InputMonitor() : mToken(nullptr), mPoll(nullptr), mEventHandler(nullptr) {}

InputMonitor::InputMonitor(const sp<IBinder> token, InputChannel* channel)
      : mToken(token), mPoll(nullptr), mEventHandler(nullptr) {
    mInputChannel.reset(channel);
}

InputMonitor::~InputMonitor() {
    FLOGI("");
    stop();
    WindowManager::releaseInput(this);
    mToken = nullptr;
    mInputChannel = nullptr;
}

void InputMonitor::stop() {
    if (mPoll) {
        FLOGI("reset poll");
        uv_poll_stop(mPoll);
        mPoll->data = nullptr;
        mEventHandler = nullptr;
        uv_close(reinterpret_cast<uv_handle_t*>(mPoll),
                 [](uv_handle_t* handle) { delete reinterpret_cast<uv_poll_t*>(handle); });
        mPoll = nullptr;
    }

    if (mInputChannel) {
        /* release input channel */
        if (mInputChannel.get()) mInputChannel->release();
        mInputChannel.reset();
    }
}

void InputMonitor::setInputChannel(InputChannel* inputChannel) {
    /* clear previous setting */
    if (inputChannel == mInputChannel.get()) return;

    stop();
    mInputChannel.reset(inputChannel);
}

bool InputMonitor::receiveMessage(const InputMessage* msg) {
    if (msg == nullptr || !isValid()) {
        FLOGW("please set input channel firstly or valid message pointer!");
        return false;
    }

    int fd = mInputChannel->getEventFd();
    ssize_t size = mq_receive(fd, (char*)msg, sizeof(InputMessage), NULL);
    if (size == sizeof(InputMessage)) {
        return true;
    }
    return false;
}

bool InputMonitor::start(uv_loop_t* loop, InputMonitorCallback callback) {
    if (callback == nullptr) {
        FLOGE("please use valid callback!");
        return false;
    }

    int fd = isValid() ? mInputChannel->getEventFd() : -1;
    if (fd == -1) {
        FLOGE("no valid file description!");
        return false;
    }

    mPoll = new uv_poll_t;
    mPoll->data = this;
    mEventHandler = callback;

    int ret = uv_poll_init(loop, mPoll, fd);
    if (ret != 0) {
        FLOGE("init monitor fd failure:%d", ret);
        return false;
    }

    ret = uv_poll_start(mPoll, UV_READABLE, [](uv_poll_t* handle, int status, int events) {
        if (status < 0) {
            FLOGE("Poll error: %s ", uv_strerror(status));
            return;
        }

        if (events & UV_READABLE) {
            InputMonitor* monitor = reinterpret_cast<InputMonitor*>(handle->data);
            if (monitor) {
                monitor->mEventHandler(monitor);
            }
        }
    });

    if (ret != 0) {
        FLOGE("start monitor(%d) failure:%d", fd, ret);
        return false;
    }

    FLOGI("start monitor(%d) success", fd);
    return true;
}

} // namespace wm
} // namespace os
