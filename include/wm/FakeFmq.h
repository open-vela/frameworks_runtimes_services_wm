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

#include <android-base/macros.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

namespace os {
namespace wm {

using android::IBinder;
using android::Parcel;
using android::Parcelable;
using android::sp;
using android::status_t;

/**
 * @class FakeFmq
 * @brief A template class for simulating FIFO message queues.
 *
 * This class provides a mock implementation of a First-In-First-Out
 * (FIFO) message queue. It includes methods for reading and writing
 * data to the queue, managing the queue's properties, and serializing
 * data for IPC (Inter-Process Communication).
 *
 * @tparam T The type of data to be stored in the queue.
 */
template <typename T>
class FakeFmq {
public:
    FakeFmq();

    ~FakeFmq();

    /**
     * @brief Reads data from the FIFO queue.
     *
     * This method reads an item from the queue if available.
     *
     * @param data Pointer to the variable where the data will be stored.
     * @return True if the read operation is successful, false otherwise.
     */
    bool read(T* data) {
        if (!mQueue || mCaps <= 0 || !data) {
            return false;
        }

        auto current = mQueue[mReadPos];

        /* no valid item */
        if (current == 0) {
            return false;
        }

        *data = current;
        mReadPos = (mReadPos + 1) % mCaps;
        return true;
    }

    /**
     * @brief Writes data to the FIFO queue.
     *
     * This method writes an item to the queue and marks the end of the
     * queue with an ending flag.
     *
     * @param data Pointer to the data to be written to the queue.
     * @return True if the write operation is successful, false otherwise.
     */
    bool write(const T* data) {
        if (!mQueue || mCaps <= 0 || !data) {
            return false;
        }

        /* firstly write ending flag */
        auto next = (mWritePos + 1) % mCaps;
        mQueue[next] = 0;

        mQueue[mWritePos] = *data;
        mWritePos = next;
        return true;
    }

    /**
     * @brief Creates the FIFO message queue.
     *
     * This method initializes the queue with the provided data and
     * sets up whether the queue operates as a server.
     *
     * @param qData Vector containing initial data for the queue.
     * @param isServer Boolean flag indicating if this queue is for a server.
     * @return True if the creation is successful, false otherwise.
     */
    bool create(const std::vector<T>& qData, bool isServer);

    /**
     * @brief Destroys the FIFO message queue.
     *
     * This method releases all resources associated with the queue.
     */
    void destroy();

    /**
     * @brief Sets the name of the FIFO message queue.
     *
     * @param name The name to be assigned to the queue.
     */
    void setName(const std::string& name) {
        mName = name;
    }

    /**
     * @brief Retrieves the name of the FIFO message queue.
     *
     * @return The name of the queue.
     */
    std::string getName() {
        return mName;
    }

    /**
     * @brief Writes the current state of the queue to a Parcel.
     *
     * @param out Pointer to the Parcel where the data will be written.
     * @return Status indicating the success or failure of the operation.
     */
    status_t writeToParcel(Parcel* out) const;

    /**
     * @brief Reads the current state of the queue from a Parcel.
     *
     * @param in Pointer to the Parcel containing the data to be read.
     * @return Status indicating the success or failure of the operation.
     */
    status_t readFromParcel(const Parcel* in);

    /**
     * @brief Copies data from another FakeFmq instance.
     *
     * @param other The other FakeFmq instance from which to copy data.
     */
    void copyFrom(FakeFmq<T>& other);

    /**
     * @brief Retrieves the current client sequence number.
     *
     * @return The client sequence number.
     */
    uint32_t getClientRespSeq() {
        return mCliRespSeq == NULL ? 0 : *mCliRespSeq;
    }

    /**
     * @brief Update the client sequence number.
     *
     * @param seq The client sequence number to be update.
     */
    void updateClientRespSeq(uint32_t seq) {
        if (mCliRespSeq == NULL) {
            return;
        }
        *mCliRespSeq = seq;
    }

    uint32_t getQueueCaps() {
        return mCaps;
    }

private:
    std::string mName;
    int mFd;
    uint32_t mCaps;
    uint32_t mReadPos;
    uint32_t mWritePos;
    uint32_t* mCliRespSeq;
    T* mQueue;
    uint32_t mQueueSize;
};

} // namespace wm
} // namespace os
