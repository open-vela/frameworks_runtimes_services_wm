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

#include "InputMonitor.h"

#include <mqueue.h>

#include "WindowUtils.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

void DefaultEventHandler::handleEvent() {
    if (!mInputMonitor) return;

    InputMessage msg;
    bool ret = mInputMonitor->receiveMessage(&msg);
    if (ret) {
        dumpInputMessage(&msg);
    }
}

InputMonitor::InputMonitor() : mToken(nullptr), mInputChannel(nullptr), mPoll(nullptr) {}

InputMonitor::InputMonitor(const sp<IBinder> token)
      : mToken(token), mInputChannel(nullptr), mPoll(nullptr) {}

InputMonitor::~InputMonitor() {
    stop();
    mToken = nullptr;
}

void InputMonitor::stop() {
    if (mPoll) {
        uv_poll_stop(mPoll);
        mPoll->data = nullptr;
        uv_close(reinterpret_cast<uv_handle_t*>(mPoll),
                 [](uv_handle_t* handle) { delete reinterpret_cast<uv_poll_t*>(handle); });
        mPoll = nullptr;
    }

    /*release old input channel*/
    if (mInputChannel) {
        mInputChannel->release();
        mInputChannel = nullptr;
    }
}

void InputMonitor::setInputChannel(InputChannel* inputChannel) {
    /* clear previous setting */
    stop();
    mInputChannel = inputChannel;
}

bool InputMonitor::receiveMessage(const InputMessage* msg) {
    if (mInputChannel == nullptr || msg == nullptr) {
        FLOGW("please set input channel firstly or valid message pointer!");
        return false;
    }

    int32_t fd = mInputChannel->getEventFd();
    if (fd == -1) {
        FLOGW("can't read message without valid channel!");
        return false;
    }

    ssize_t size = mq_receive(fd, (char*)msg, sizeof(InputMessage), NULL);
    return size == sizeof(InputMessage) ? true : false;
}

void InputMonitor::start(uv_loop_t* loop, EventHandler* handler) {
    int fd = mInputChannel ? mInputChannel->getEventFd() : -1;
    if (fd == -1) {
        FLOGW("no valid file description!");
        return;
    }

    mPoll = new uv_poll_t;
    mPoll->data = handler;
    uv_poll_init(loop, mPoll, fd);
    uv_poll_start(mPoll, UV_READABLE, [](uv_poll_t* handle, int status, int events) {
        if (status < 0) {
            FLOGE("Poll error: %s ", uv_strerror(status));
            return;
        }

        if (events & UV_READABLE) {
            EventHandler* tmpHandle = reinterpret_cast<EventHandler*>(handle->data);
            if (tmpHandle) {
                tmpHandle->handleEvent();
            }
        }
    });
}

} // namespace wm
} // namespace os
