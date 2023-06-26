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

BufferQueue::BufferQueue(const std::shared_ptr<SurfaceControl>& sc) {
    update(sc);
}

BufferQueue::~BufferQueue() {
    // TODO:
}

bool BufferQueue::update(const std::shared_ptr<SurfaceControl>& sc) {
    // TODO: same SurfaceControl, should return directly

    // TODO: clear buffers

    mSurfaceControl = sc;

    // TODO: update buffers and Slot
    return true;
}

bool BufferQueue::toState(BufferItem* item, BufferState state) {
    // TODO: state machine
    return false;
}

BufferItem* BufferQueue::getBuffer(BufferSlot slot) {
    BufferKey key = -1;

    if (slot == BSLOT_FREE && !mFreeSlot.empty()) {
        key = mFreeSlot.front();
    } else if (slot == BSLOT_DATA && !mDataSlot.empty()) {
        key = mDataSlot.front();
    }

    if (key != 0 && mBuffers.find(key) != mBuffers.end()) {
        return &mBuffers[key];
    }

    return nullptr;
}

} // namespace wm
} // namespace os
