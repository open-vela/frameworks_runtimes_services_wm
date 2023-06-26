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

#include <memory>
#include <queue>
#include <unordered_map>

namespace os {
namespace wm {

class SurfaceControl;

typedef int BufferKey;
typedef struct {
    BufferKey mKey;
    int mFd;
} BufferId;

typedef struct {
    BufferKey mKey;
    int mFd;
    void* mBuffer;
    int mSize;
} BufferItem;

typedef enum {
    BSTATE_FREE = 0,
    BSTATE_DEQUEUED,
    BSTATE_QUEUED,
    BSTATE_ACQUIRED,
} BufferState;

typedef enum {
    BSLOT_FREE = 0,
    BSLOT_DATA,
} BufferSlot;

class BufferQueue {
public:
    BufferQueue(const std::shared_ptr<SurfaceControl>& sc);
    ~BufferQueue();

    bool update(const std::shared_ptr<SurfaceControl>& sc);

protected:
    BufferItem* getBuffer(BufferSlot slot);
    bool toState(BufferItem* item, BufferState state);

private:
    std::weak_ptr<SurfaceControl> mSurfaceControl;
    std::unordered_map<BufferKey, BufferItem> mBuffers;

    std::queue<BufferKey> mDataSlot;
    std::queue<BufferKey> mFreeSlot;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
};

class BufferProducer : public BufferQueue {
public:
    BufferProducer(const std::shared_ptr<SurfaceControl>& sc);
    ~BufferProducer();

    BufferItem* dequeueBuffer();
    int queueBuffer(BufferItem* buffer);
};

class BufferConsumer : public BufferQueue {
public:
    BufferConsumer(const std::shared_ptr<SurfaceControl>& sc);
    ~BufferConsumer();

    BufferItem* acquireBuffer();
    int releaseBuffer(BufferItem* buffer);
};

} // namespace wm
} // namespace os
