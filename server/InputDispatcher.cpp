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

#define LOG_TAG "InputDispatcher"

#include "InputDispatcher.h"

#include <mqueue.h>

#include "WindowUtils.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

InputDispatcher::InputDispatcher() {}

InputDispatcher::~InputDispatcher() {
    release();
}

bool InputDispatcher::create(const std::string name) {
    return mInputChannel.create(name);
}

void InputDispatcher::release() {
    return mInputChannel.release();
}

bool InputDispatcher::sendMessage(const InputMessage* ie) {
    int fd = mInputChannel.getEventFd();
    if (fd == -1) {
        FLOGW("input dispatcher: can't send message without valid channel!");
        return false;
    }

    int ret = mq_send(fd, (const char*)ie, sizeof(InputMessage), 100);
    if (ret < 0) {
        FLOGW("input dispatcher: send message failed:'%s(%d)'", strerror(errno), errno);
        return false;
    }
    return true;
}

} // namespace wm
} // namespace os
