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

namespace os {
namespace wm {

using namespace android;
using namespace android::base;
using namespace android::binder;
using namespace std;

class InputDispatcher {
public:
    InputDispatcher();
    ~InputDispatcher();

    bool create(const std::string name);
    void release();

    bool sendMessage(const InputMessage* ie);

    DISALLOW_COPY_AND_ASSIGN(InputDispatcher);

    InputChannel& getInputChannel() {
        return mInputChannel;
    }

private:
    InputChannel mInputChannel;
};

} // namespace wm
} // namespace os
