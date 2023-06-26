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

#include <sys/mman.h>

#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

BufferQueue::BufferQueue(const std::shared_ptr<SurfaceControl>& sc) {
    update(sc);
}

BufferQueue::~BufferQueue() {
    mSurfaceControl.reset();

    if (!mBuffers.empty()) {
        clearBuffers();
    }

    mDataSlot.clear();
    mFreeSlot.clear();
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

void BufferQueue::clearBuffers() {
    for (auto it = mBuffers.begin(); it != mBuffers.end(); ++it) {
        if (it->second.mBuffer && munmap(it->second.mBuffer, it->second.mSize) == -1) {
            ALOGE("munmap");
        }

        if (close(it->second.mFd) == -1) {
            ALOGE("close");
        }
    }
    mBuffers.clear();
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
    if (sc->isSameSurface(sc, mSurfaceControl.lock())) {
        return false;
    }

    if (!mBuffers.empty()) {
        clearBuffers();
    }

    mSurfaceControl = sc;

    std::unordered_map<BufferKey, BufferId> bufferIds = sc->bufferIds();
    uint32_t width = sc->getWidth();
    uint32_t height = sc->getHeight();
    // uint32_t format = sc->getFormat();
    // TODO:parse mformat, temporary value is 4 bytes
    uint32_t size = width * height * 4;

    for (auto it = bufferIds.begin(); it != bufferIds.end(); ++it) {
        BufferKey bufferkey = it->first;
        int bufferFd = it->second.mFd;

        if (bufferFd == -1) {
            return false;
        }

        if (ftruncate(bufferFd, size) == -1) {
            ALOGE("Failed to resize shared memory");
            return false;
        }

        void* buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, bufferFd, 0);
        if (buffer == MAP_FAILED) {
            ALOGE("Failed to map shared memory");
            return false;
        }
        BufferItem buffItem = {bufferkey, bufferFd, buffer, size, BSTATE_FREE};
        mBuffers[bufferkey] = buffItem;
    }
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
                // check it in free slot
                auto it = std::find(mFreeSlot.begin(), mFreeSlot.end(), item->mKey);
                if (it == mFreeSlot.end()) {
                    return false;
                }
                // remove from free slot and update state
                mFreeSlot.remove(item->mKey);
                item->mState = state;
                return true;
            } else if (state == BSTATE_QUEUED) {
                // move it from free slot to data slot
                auto it = std::find(mFreeSlot.begin(), mFreeSlot.end(), item->mKey);
                if (it == mFreeSlot.end()) {
                    return false;
                }
                mFreeSlot.remove(item->mKey);
                mDataSlot.push_back(item->mKey);
                item->mState = state;
                return true;
            }
            break;

        case BSTATE_DEQUEUED:
            if (state == BSTATE_QUEUED) {
                // move to data slot and update state
                auto it = std::find(mDataSlot.begin(), mDataSlot.end(), item->mKey);
                if (it != mDataSlot.end()) {
                    return false;
                }
                mDataSlot.push_back(item->mKey);
                item->mState = state;
                return true;
            } else if (state == BSTATE_FREE) {
                // move to free slot and update state
                auto it = std::find(mFreeSlot.begin(), mFreeSlot.end(), item->mKey);
                if (it != mFreeSlot.end()) {
                    return false;
                }
                mFreeSlot.push_back(item->mKey);
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
                // move it from data slot to free slot and update state
                auto it = std::find(mDataSlot.begin(), mDataSlot.end(), item->mKey);
                if (it == mDataSlot.end()) {
                    return false;
                }
                mDataSlot.remove(item->mKey);
                mFreeSlot.push_back(item->mKey);
                item->mState = state;
                return true;
            }
            break;

        case BSTATE_ACQUIRED:
            if (state == BSTATE_FREE) {
                // move it from data slot to free slot and update state
                auto it = std::find(mDataSlot.begin(), mDataSlot.end(), item->mKey);
                if (it == mDataSlot.end()) {
                    return false;
                }
                mDataSlot.remove(item->mKey);
                mFreeSlot.push_back(item->mKey);
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
