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

#include <unordered_map>

#include "wm/BufferQueue.h"
#include "wm/FakeFmq.h"

/**
 * @namespace os::wm
 * @brief The namespace for the window manager classes and interfaces.
 */
namespace os {
namespace wm {

using android::IBinder;
using android::Parcel;
using android::Parcelable;
using android::sp;
using android::status_t;

/**
 * @brief Type definition for surface free information.
 */
typedef FakeFmq<BufferKey> SurfaceFreeInfoClass;

/**
 * @class SurfaceControl
 * @brief Class representing a surface in the window manager.
 *
 * This class encapsulates the properties and behaviors of a rendering
 * surface. It manages the buffer handling, drawing operations, and
 * communication with the window manager.
 */
class SurfaceControl : public Parcelable {
public:
    SurfaceControl();

    ~SurfaceControl();

    SurfaceControl(const sp<IBinder>& token, const sp<IBinder>& handle, uint32_t width = 0,
                   uint32_t height = 0, uint32_t format = 0, uint32_t size = 0);

    /**
     * @brief Writes the SurfaceControl data to a Parcel.
     *
     * This method overrides the writeToParcel method from the Parcelable
     * interface.
     *
     * @param out Pointer to the Parcel where the data will be written.
     * @return Status indicating the success or failure of the operation.
     */
    status_t writeToParcel(Parcel* out) const override;

    /**
     * @brief Reads the SurfaceControl data from a Parcel.
     *
     * This method overrides the readFromParcel method from the Parcelable
     * interface.
     *
     * @param in Pointer to the Parcel from which the data will be read.
     * @return Status indicating the success or failure of the operation.
     */
    status_t readFromParcel(const Parcel* in) override;

    /**
     * @brief Checks if the surface control is valid.
     *
     * @return True if the surface control is valid, false otherwise.
     */
    bool isValid();

    std::vector<BufferId>& bufferIds() {
        return mBufferIds;
    }

    /**
     * @brief Initializes the buffer IDs for the surface control.
     *
     * @param ids Vector of BufferId to initialize with.
     */
    void initBufferIds(const std::vector<BufferId>& ids);

    /**
     * @brief Clears the buffer IDs associated with the surface.
     */
    void clearBufferIds() {
        mBufferIds.clear();
    }

    /**
     * @brief Sets the buffer queue for the surface control.
     *
     * @param bq Shared pointer to the BufferQueue to be set.
     */
    void setBufferQueue(const std::shared_ptr<BufferQueue>& bq) {
        mBufferQueue = bq;
    }

    /**
     * @brief Retrieves the buffer queue associated with the surface control.
     *
     * @return Shared pointer to the BufferQueue.
     */
    std::shared_ptr<BufferQueue> bufferQueue() {
        return mBufferQueue;
    }

    /**
     * @brief Retrieves the token associated with the surface control.
     *
     * @return Shared pointer to the IBinder token.
     */
    sp<IBinder> getToken() {
        return mToken;
    }

    /**
     * @brief Retrieves the handle associated with the surface control.
     *
     * @return Shared pointer to the IBinder handle.
     */
    sp<IBinder> getHandle() {
        return mHandle;
    }

    /**
     * @brief Retrieves the width of the surface control.
     *
     * @return Width of the surface.
     */
    uint32_t getWidth() {
        return mWidth;
    }

    /**
     * @brief Retrieves the height of the surface control.
     *
     * @return Height of the surface.
     */
    uint32_t getHeight() {
        return mHeight;
    }

    /**
     * @brief Retrieves the format of the surface control.
     *
     * @return Format of the buffer.
     */
    uint32_t getFormat() {
        return mFormat;
    }

    /**
     * @brief Retrieves the size of the buffers used by the surface control.
     *
     * @return Size of the buffer.
     */
    uint32_t getBufferSize() {
        return mBufferSize;
    }

    /**
     * @brief Checks if two SurfaceControl instances represent the same surface.
     *
     * @param lhs Shared pointer to the first SurfaceControl.
     * @param rhs Shared pointer to the second SurfaceControl.
     * @return True if both SurfaceControls are the same surface, false otherwise.
     */
    static bool isSameSurface(const std::shared_ptr<SurfaceControl>& lhs,
                              const std::shared_ptr<SurfaceControl>& rhs);

    /**
     * @brief Copies data from another SurfaceControl instance.
     *
     * @param other The SurfaceControl instance from which data will be copied.
     */
    void copyFrom(SurfaceControl& other);

    /**
     * @brief Initializes the Fast Message Queue (FMQ) for input/output.
     *
     * @param isServer Boolean flag indicating if this instance is a server.
     * @return True if initialization is successful, false otherwise.
     */
    bool initFMQ(bool isServer);

    /**
     * @brief Destroys the Fast Message Queue (FMQ) resources.
     */
    void destroyFMQ();

    /**
     * @brief Retrieves the FMQ used for managing free message slots.
     *
     * @return Reference to the SurfaceFreeInfoClass.
     */
    SurfaceFreeInfoClass& getFMQ() {
        return mFreeMsgSlot;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SurfaceControl);

    sp<IBinder> mToken;
    sp<IBinder> mHandle;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
    uint32_t mBufferSize;
    std::vector<BufferId> mBufferIds;
    std::shared_ptr<BufferQueue> mBufferQueue;
    SurfaceFreeInfoClass mFreeMsgSlot;
};

/**
 * @brief Initializes the surface buffer.
 *
 * @param sc A shared pointer to the SurfaceControl.
 * @param isServer A boolean indicating if the initialization is for the server side.
 */
void initSurfaceBuffer(const std::shared_ptr<SurfaceControl>& sc, bool isServer);

/**
 * @brief Uninitializes the surface buffer.
 *
 * @param sc A shared pointer to the SurfaceControl. It must provide a valid reference when calling
 * this function.
 */
void uninitSurfaceBuffer(const std::shared_ptr<SurfaceControl>& sc);

} // namespace wm
} // namespace os
