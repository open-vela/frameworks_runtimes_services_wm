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

std::shared_ptr<InputDispatcher> InputDispatcher::create(const std::string name) {
    WM_PROFILER_BEGIN();
    auto dispatcher = std::make_shared<InputDispatcher>();
    if (!dispatcher->getInputChannel().create(name)) {
        dispatcher = nullptr;
    }
    WM_PROFILER_END();
    return dispatcher;
}

void InputDispatcher::release() {
    return mInputChannel.release();
}

int InputDispatcher::sendMessage(const InputMessage* ie) {
    int fd = mInputChannel.getEventFd();
    if (fd == -1) {
        FLOGW("can't send message without valid channel!");
        return -1;
    }

    int ret = mq_send(fd, (const char*)ie, sizeof(InputMessage), 100);

    if (ret < 0) {
        FLOGW("send message to %d, failed: %d - '%s(%d)'", fd, ret, strerror(errno), errno);
        return ret;
    }
    return 0;
}

} // namespace wm
} // namespace os
