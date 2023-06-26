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

class LayerState : public Parcelable {
public:
    LayerState() : mFlags(0), mToken(nullptr) {}
    ~LayerState() {
        mToken = nullptr;
        mFlags = 0;
    }

    LayerState(sp<IBinder> token) : mFlags(0), mToken(token) {}

    status_t writeToParcel(Parcel* out) const override;
    status_t readFromParcel(const Parcel* in) override;

    void merge(LayerState& state);

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
};

} // namespace wm
} // namespace os
