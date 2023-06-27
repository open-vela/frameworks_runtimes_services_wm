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

#define LOG_TAG "WindowState"

#include "WindowState.h"

#include <mqueue.h>
#include <sys/mman.h>
#include <utils/RefBase.h>

#include "wm/InputMessage.h"

namespace os {
namespace wm {

WindowState::WindowState(WindowManagerService* service, const sp<IWindow>& window,
                         WindowToken* token, const LayoutParams& params, int32_t visibility)
      : mClient(window), mToken(token), mService(service), mInputChannel(nullptr) {
    mAttrs = params;
    mVisibility = visibility != 0 ? true : false;
}

std::shared_ptr<InputChannel> WindowState::createInputChannel(const std::string name) {
    if (mInputChannel != nullptr) {
        ALOGE("mInputChannel is existed,create failed");
        return nullptr;
    }
    struct mq_attr mqstat;
    int oflag = O_CREAT | O_RDWR | O_NONBLOCK;
    mqd_t messageQueue = 0;

    memset(&mqstat, 0, sizeof(mqstat));
    mqstat.mq_maxmsg = MAX_MSG;
    mqstat.mq_msgsize = sizeof(InputMessage);
    mqstat.mq_flags = 0;

    if (((mqd_t)-1) == (messageQueue = mq_open(name.c_str(), oflag, 0777, &mqstat))) {
        ALOGI("mq_open doesn't return success ");
        return nullptr;
    }

    mInputChannel = std::make_shared<InputChannel>();
    mInputChannel->setEventFd(messageQueue);
    return mInputChannel;
}

} // namespace wm
} // namespace os