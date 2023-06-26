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

#include "wm/LayerState.h"

#include "wm/Rect.h"

namespace os {
namespace wm {

status_t LayerState::writeToParcel(Parcel* out) const {
    SAFE_PARCEL(out->writeStrongBinder, mToken);
    SAFE_PARCEL(out->writeInt32, mFlags);

    if (mFlags & LAYER_POSITION_CHANGED) {
        SAFE_PARCEL(out->writeInt32, mX);
        SAFE_PARCEL(out->writeInt32, mY);
    }

    if (mFlags & LAYER_BUFFER_CHANGED) {
        SAFE_PARCEL(out->writeInt32, mBufferKey);
    }

    if (mFlags & LAYER_BUFFER_CROP_CHANGED) {
        mBufferCrop.writeToParcel(out);
    }

    if (mFlags & LAYER_ALPHA_CHANGED) {
        SAFE_PARCEL(out->writeInt32, mAlpha);
    }
    return android::OK;
}

status_t LayerState::readFromParcel(const Parcel* in) {
    SAFE_PARCEL(in->readStrongBinder, &mToken);
    SAFE_PARCEL(in->readInt32, &mFlags);

    if (mFlags & LAYER_POSITION_CHANGED) {
        SAFE_PARCEL(in->readInt32, &mX);
        SAFE_PARCEL(in->readInt32, &mY);
    }

    if (mFlags & LAYER_BUFFER_CHANGED) {
        SAFE_PARCEL(in->readInt32, &mBufferKey);
    }

    if (mFlags & LAYER_BUFFER_CROP_CHANGED) {
        mBufferCrop.readFromParcel(in);
    }

    if (mFlags & LAYER_ALPHA_CHANGED) {
        SAFE_PARCEL(in->readInt32, &mAlpha);
    }
    return android::OK;
}

} // namespace wm
} // namespace os
