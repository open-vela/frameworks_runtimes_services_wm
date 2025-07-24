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
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

namespace os {
namespace wm {

using namespace android;
using namespace android::base;
using namespace android::binder;
using namespace std;

/**
 * @class InputChannel
 * @brief Class for handling input events in the window manager.
 *
 * This class represents an input channel that is responsible for
 * managing input events and facilitating communication between
 * the input event producer and consumer in the window manager.
 */
class InputChannel : public Parcelable {
public:
    InputChannel();

    ~InputChannel();

    /**
     * @brief Writes the input channel information to a Parcel.
     *
     * This method overrides the writeToParcel method from the Parcelable interface.
     *
     * @param out Pointer to the Parcel where data will be written.
     * @return Status indicating the success or failure of the operation.
     */
    status_t writeToParcel(Parcel* out) const override;

    /**
     * @brief Reads the input channel information from a Parcel.
     *
     * This method overrides the readFromParcel method from the Parcelable interface.
     *
     * @param in Pointer to the Parcel from which data will be read.
     * @return Status indicating the success or failure of the operation.
     */
    status_t readFromParcel(const Parcel* in) override;

    /**
     * @brief Retrieves the file descriptor for the event channel.
     *
     * @return The file descriptor used for input events.
     */
    int getEventFd() {
        return mEventFd;
    }

    /**
     * @brief Sets the file descriptor for the input channel.
     *
     * @param fd The file descriptor to be set for the input channel.
     */
    void setEventFd(int fd) {
        mEventFd = fd;
    }

    /**
     * @brief Copies the contents from another InputChannel instance.
     *
     * @param other The InputChannel instance from which to copy data.
     */
    void copyFrom(InputChannel& other) {
        mEventFd = other.mEventFd;
        mEventName = other.mEventName;
    }

    /**
     * @brief Checks if the input channel is valid.
     *
     * @return True if the input channel is valid, false otherwise.
     */
    bool isValid();

    /**
     * @brief Creates the input channel.
     *
     * This method initializes the input channel with the specified
     * name and allocates resources needed for input event handling.
     *
     * @param name The name of the input channel.
     * @return True if the creation is successful, false otherwise.
     */
    bool create(const std::string& name);

    /**
     * @brief Releases the resources associated with the input channel.
     *
     * This method cleans up and deallocates any resources used by the
     * input channel, preparing it for destruction or reuse.
     */
    void release();

    DISALLOW_COPY_AND_ASSIGN(InputChannel);

private:
    int mEventFd;
    std::string mEventName;
};

} // namespace wm
} // namespace os
