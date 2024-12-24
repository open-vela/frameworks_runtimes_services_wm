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

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include "wm/BufferQueue.h"
#include "wm/Rect.h"

namespace os {
namespace wm {

using android::IBinder;
using android::Parcelable;
using android::sp;
using android::status_t;

/**
 * @class LayerState
 * @brief Represents the state of a layer in the window manager.
 *
 * This class encapsulates the properties of a layer, including
 * the buffer state, position, alpha, and related attributes.
 * It is used to manage layer updates and communication between
 * the window manager and the rendering engine.
 */
class LayerState : public Parcelable {
public:
    LayerState() : mFlags(0), mToken(nullptr), mSeq(0) {}

    ~LayerState() {
        mToken = nullptr;
        mFlags = 0;
    }

    LayerState(sp<IBinder> token) : mFlags(0), mToken(token) {}

    /**
     * @brief Writes the LayerState data to a Parcel.
     *
     * This method overrides the writeToParcel method from the Parcelable
     * interface.
     *
     * @param out Pointer to the Parcel where data will be written.
     * @return Status indicating the success or failure of the operation.
     */
    status_t writeToParcel(Parcel* out) const override;

    /**
     * @brief Reads LayerState data from a Parcel.
     *
     * This method overrides the readFromParcel method from the Parcelable
     * interface.
     *
     * @param in Pointer to the Parcel from which data will be read.
     * @return Status indicating the success or failure of the operation.
     */
    status_t readFromParcel(const Parcel* in) override;

    /**
     * @brief Merges another LayerState into this LayerState.
     *
     * This method combines the properties of the given LayerState into
     * the current LayerState.
     *
     * @param state The LayerState to merge with the current state.
     */
    void merge(LayerState& state);

    /**
     * @brief Enumeration of flags representing layer property changes.
     */
    enum {
        LAYER_POSITION_CHANGED = 0x01,
        LAYER_ALPHA_CHANGED = 0x02,
        LAYER_BUFFER_CHANGED = 0x04,
        LAYER_BUFFER_CROP_CHANGED = 0x08,
    };

    int32_t mX;
    int32_t mY;
    int32_t mAlpha;
    BufferKey mBufferKey;
    Rect mBufferCrop;
    int32_t mFlags;
    sp<IBinder> mToken;
    uint32_t mSeq;
};

} // namespace wm
} // namespace os
