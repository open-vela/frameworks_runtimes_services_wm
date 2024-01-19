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

#include <utils/Log.h>

#define MAX_MSG 50

namespace os {
namespace wm {

typedef enum {
    INPUT_MESSAGE_TYPE_NONE,
    INPUT_MESSAGE_TYPE_POINTER,
    INPUT_MESSAGE_TYPE_KEYPAD,
    INPUT_MESSAGE_TYPE_BUTTON,
    INPUT_MESSAGE_TYPE_ENCODER,
} InputMessageType;

typedef enum { INPUT_MESSAGE_STATE_RELEASED = 0, INPUT_MESSAGE_STATE_PRESSED } InputMessageState;

typedef struct {
    InputMessageType type;
    InputMessageState state;
    union {
        struct {
            uint32_t key_code;
        } keypad;
        struct {
            int32_t x, y;
            int32_t raw_x, raw_y;
        } pointer;
    };
} InputMessage;

static inline void dumpInputMessage(const InputMessage* ie) {
    if (!ie) return;

    ALOGD("Message: type(%d), state(%d)", ie->type, ie->state);
    if (ie->type == INPUT_MESSAGE_TYPE_POINTER) {
        ALOGD("\t\traw pos(%" PRId32 ", %" PRId32 "), pos(%" PRId32 ", %" PRId32 ")",
              ie->pointer.raw_x, ie->pointer.raw_y, ie->pointer.x, ie->pointer.y);
    } else if (ie->type == INPUT_MESSAGE_TYPE_KEYPAD) {
        ALOGD("\t\tkeycode(%" PRId32 ")", ie->keypad.key_code);
    }
}

} // namespace wm
} // namespace os
