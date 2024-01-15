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

#define LOG_TAG "InputChannel"

#include "wm/InputChannel.h"

#include <mqueue.h>

#include "WindowUtils.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

InputChannel::InputChannel() : mEventFd(-1), mEventName("") {}

InputChannel::~InputChannel() {}

status_t InputChannel::writeToParcel(Parcel* out) const {
    status_t result = out->writeFileDescriptor(mEventFd);
    SAFE_PARCEL(out->writeCString, mEventName.c_str());

    return result;
}

status_t InputChannel::readFromParcel(const Parcel* in) {
    mEventFd = dup(in->readFileDescriptor());
    mEventName = in->readCString();

    return android::OK;
}

bool InputChannel::create(const std::string name) {
    const char* cname = name.c_str();
    struct mq_attr mqstat;
    int oflag = O_CREAT | O_RDWR | O_NONBLOCK;

    memset(&mqstat, 0, sizeof(mqstat));
    mqstat.mq_maxmsg = MAX_MSG;
    mqstat.mq_msgsize = sizeof(InputMessage);
    mqstat.mq_flags = 0;

    if (((mqd_t)-1) == (mEventFd = mq_open(cname, oflag, 0777, &mqstat))) {
        FLOGW("Failed with '%s', error: %d", cname, errno);
        return false;
    }
    mEventName = name;

    return true;
}

void InputChannel::release() {
    if (isValid()) {
        mq_close(mEventFd);
        mq_unlink(mEventName.c_str());
        FLOGI("mq unlink:%s", mEventName.c_str());
        mEventFd = -1;
        mEventName = "";
    }
}

} // namespace wm
} // namespace os
