
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

#include "wm/BufferQueue.h"
#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

BufferConsumer::BufferConsumer(const std::shared_ptr<SurfaceControl>& sc) : BufferQueue(sc) {}
BufferConsumer::~BufferConsumer() {}

BufferItem* BufferConsumer::acquireBuffer() {
    BufferItem* bufferItem = getBuffer(BSLOT_DATA);
    if (bufferItem != nullptr && toState(bufferItem, BSTATE_ACQUIRED)) {
        return bufferItem;
    }
    return nullptr;
}

int BufferConsumer::releaseBuffer(BufferItem* buffer) {
    if (buffer == nullptr) {
        return false;
    }
    return toState(buffer, BSTATE_FREE);
}

} // namespace wm
} // namespace os