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

BufferItem* BufferQueue::getBuffer(BufferKey bufKey) {
    if (mBuffers.find(bufKey) != mBuffers.end()) {
        return &mBuffers[bufKey];
    }
    return nullptr;
}

// after dequeue or acqure buffer, allow to cancel buffer
bool BufferQueue::cancelBuffer(BufferItem* item) {
    return toState(item, BSTATE_FREE);
}

bool BufferQueue::syncState(BufferKey key, BufferState byState) {
    BufferItem* item = getBuffer(key);
    if (!item) return false;

    if (item->mState == BSTATE_QUEUED && byState == BSTATE_FREE) {
        // sync consumer free byState to producer
        return toState(item, BSTATE_FREE);
    } else if (item->mState == BSTATE_FREE && byState == BSTATE_QUEUED) {
        // sync producer queue byState to consumer
        return toState(item, BSTATE_QUEUED);
    }
    return false;
}

bool BufferQueue::update(const std::shared_ptr<SurfaceControl>& sc) {
    // TODO: same SurfaceControl, should return directly

    // TODO: clear buffers

    mSurfaceControl = sc;

    // TODO: update buffers and Slot
    return true;
}

bool BufferQueue::toState(BufferItem* item, BufferState state) {
    /*
     * PRODUCER: FREE <-> DEQUEUED -> QUEUED -> FREE
     * CONSUMER: FREE <-> QUEUED -> ACQUIRED -> FREE
     */
    switch (item->mState) {
        case BSTATE_FREE:
            if (state == BSTATE_DEQUEUED) {
                // TODO: check it in free slot

                // remove from free slot and update state
                mFreeSlot.remove(item->mKey);
                item->mState = state;
                return true;
            } else if (state == BSTATE_QUEUED) {
                // TODO: move it from free slot to data slot
                item->mState = state;
                return true;
            }
            break;

        case BSTATE_DEQUEUED:
            if (state == BSTATE_QUEUED) {
                // TODO: move to data slot and update state
                item->mState = state;
                return true;
            } else if (state == BSTATE_FREE) {
                // TODO: move to free slot and update state
                item->mState = state;
                return true;
            }
            break;

        case BSTATE_QUEUED:
            if (state == BSTATE_ACQUIRED) {
                // update state, buffer in data slot
                item->mState = state;
                return true;
            } else if (state == BSTATE_FREE) {
                // TODO: move to free slot and update state
                item->mState = state;
                return true;
            }
            break;

        case BSTATE_ACQUIRED:
            if (state == BSTATE_FREE) {
                // TODO: move it from data slot to free slot and update state
                item->mState = state;
                return true;
            }
            break;

        default:
            break;
    }
    return false;
}

BufferItem* BufferQueue::getBuffer(BufferSlot slot) {
    BufferKey key = -1;

    if (slot == BSLOT_FREE && !mFreeSlot.empty()) {
        key = mFreeSlot.front();
    } else if (slot == BSLOT_DATA && !mDataSlot.empty()) {
        key = mDataSlot.front();
    }

    return getBuffer(key);
}

} // namespace wm
} // namespace os
