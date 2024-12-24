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

#include <nuttx/config.h>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

namespace os {
namespace wm {

class SurfaceControl;

typedef enum {
    BSTATE_FREE = 0,
    BSTATE_DEQUEUED,
    BSTATE_QUEUED,
    BSTATE_ACQUIRED,
} BufferState;

/**
 * @brief Type definition for BufferKey.
 */
typedef int32_t BufferKey;

/**
 * @struct BufferId
 * @brief Structure representing an identifier for a buffer.
 *
 * This structure contains the name, key, and file descriptor associated with a buffer.
 */
typedef struct {
    std::string mName;
    BufferKey mKey;
    int mFd;
} BufferId;

/**
 * @struct BufferItem
 * @brief Structure representing a buffer item.
 *
 * This structure contains information about a buffer, including its key,
 * file descriptor, raw pointer, size, state, and user data.
 */
typedef struct {
    BufferKey mKey;
    int mFd;
    void* mBuffer;
    uint32_t mSize;
    BufferState mState;
    void* mUserData;
} BufferItem;

/**
 * @brief Enumeration defining the buffer slot types.
 */
typedef enum {
    BSLOT_FREE = 0,
    BSLOT_DATA,
} BufferSlot;

/**
 * @class BufferQueue
 * @brief Class managing a queue of buffer items.
 *
 * This class provides functionality to manage buffers used in the
 * rendering process. It handles buffer allocation, state management,
 * and synchronization between producers and consumers.
 */
class BufferQueue {
public:
    BufferQueue(const std::shared_ptr<SurfaceControl>& sc);

    virtual ~BufferQueue();

    /**
     * @brief Updates the BufferQueue with new information from the SurfaceControl.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @return True if the update is successful, false otherwise.
     */
    bool update(const std::shared_ptr<SurfaceControl>& sc);

    /**
     * @brief Cancels the specified buffer.
     *
     * @param item Pointer to the BufferItem to be canceled.
     * @return True if the cancellation is successful, false otherwise.
     */
    bool cancelBuffer(BufferItem* item);

protected:
    /**
     * @brief Retrieves a buffer from the specified slot.
     *
     * @param slot The slot from which to retrieve the buffer.
     * @return Pointer to the BufferItem in the specified slot.
     */
    BufferItem* getBuffer(BufferSlot slot);

    /**
     * @brief Synchronizes the state of a buffer based on its key.
     *
     * @param key The key identifying the buffer.
     * @param state The new state to set for the buffer.
     * @return Pointer to the BufferItem after synchronization.
     */
    BufferItem* syncState(BufferKey key, BufferState state);

    /**
     * @brief Changes the state of a specified buffer item.
     *
     * @param item Pointer to the BufferItem to update.
     * @param state The new state to assign to the item.
     * @return True if the state change is successful, false otherwise.
     */
    bool toState(BufferItem* item, BufferState state);

private:
    BufferItem* getBuffer(BufferKey bufKey);

    void clearBuffers();

    std::weak_ptr<SurfaceControl> mSurfaceControl;
    std::unordered_map<BufferKey, BufferItem> mBuffers;

    std::list<BufferKey> mDataSlot;
    std::list<BufferKey> mFreeSlot;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
};

/**
 * @class BufferProducer
 * @brief Class for producing buffers in the BufferQueue.
 *
 * This class provides functionality to dequeue and queue buffers
 * for rendering purposes.
 */
class BufferProducer : public BufferQueue {
public:
    BufferProducer(const std::shared_ptr<SurfaceControl>& sc);

    ~BufferProducer();

    /**
     * @brief Dequeues a buffer for use.
     *
     * @return Pointer to the dequeued BufferItem.
     */
    BufferItem* dequeueBuffer();

    /**
     * @brief Queues a buffer for processing.
     *
     * @param buffer Pointer to the BufferItem to be queued.
     * @return True if the queuing operation is successful, false otherwise.
     */
    bool queueBuffer(BufferItem* buffer);

    /**
     * @brief Synchronizes the free state of a buffer based on its key.
     *
     * @param key The key identifying the buffer to synchronize.
     * @return Pointer to the BufferItem after synchronization.
     */
    BufferItem* syncFreeState(BufferKey key) {
        return syncState(key, BSTATE_FREE);
    }
};

/**
 * @class BufferConsumer
 * @brief Class for consuming buffers in the BufferQueue.
 *
 * This class provides functionality to acquire and release buffers
 * that have been produced.
 */
class BufferConsumer : public BufferQueue {
public:
    BufferConsumer(const std::shared_ptr<SurfaceControl>& sc);

    ~BufferConsumer();

    /**
     * @brief Acquires a buffer for processing.
     *
     * @return Pointer to the acquired BufferItem.
     */
    BufferItem* acquireBuffer();

    /**
     * @brief Releases a buffer back to the queue.
     *
     * @param buffer Pointer to the BufferItem to be released.
     * @return True if the release is successful, false otherwise.
     */
    bool releaseBuffer(BufferItem* buffer);

    /**
     * @brief Synchronizes the queued state of a buffer based on its key.
     *
     * @param key The key identifying the buffer to synchronize.
     * @return Pointer to the BufferItem after synchronization.
     */
    BufferItem* syncQueuedState(BufferKey key) {
        return syncState(key, BSTATE_QUEUED);
    }
};

} // namespace wm
} // namespace os
