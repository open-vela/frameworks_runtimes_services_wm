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

#include "DummyDriverProxy.h"

namespace os {
namespace wm {

DummyDriverProxy::DummyDriverProxy(std::shared_ptr<BaseWindow> win) : UIDriverProxy(win) {
    initUIInstance();
}

DummyDriverProxy::~DummyDriverProxy() {}

bool DummyDriverProxy::initUIInstance() {
    mActive = false;
    return true;
}

void DummyDriverProxy::drawFrame(BufferItem* bufItem) {
    UIDriverProxy::drawFrame(bufItem);
    ALOGI("draw frame for dummy proxy.");

    if (!bufItem) return;

    void* buffer = onDequeueBuffer();
    if (buffer && mEventCallback) {
        mEventCallback(buffer, bufItem->mSize, MOCKUI_EVENT_DRAW);
    }
    onQueueBuffer();
}

void DummyDriverProxy::handleEvent(InputMessage& message) {
    dumpInputMessage(&message);

    switch (message.type) {
        case INPUT_MESSAGE_TYPE_POINTER: {
            if (message.state == INPUT_MESSAGE_STATE_PRESSED) {
                mActive = true;
            } else if (message.state == INPUT_MESSAGE_STATE_RELEASED) {
                if (!mActive) {
                    return;
                }

                mActive = false;
                if (mEventCallback) mEventCallback(NULL, 0, MOCKUI_EVENT_CLICK);
            }
            break;
        }

        default:
            mActive = false;
            break;
    }
}

void* DummyDriverProxy::getRoot() {
    return nullptr;
}

void* DummyDriverProxy::getWindow() {
    return nullptr;
}

} // namespace wm
} // namespace os
