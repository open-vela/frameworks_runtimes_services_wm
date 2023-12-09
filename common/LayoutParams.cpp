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

#define LOG_TAG "LayoutParams"

#include "wm/LayoutParams.h"

#include "ParcelUtils.h"

namespace os {
namespace wm {

LayoutParams::LayoutParams() {
    mWidth = MATCH_PARENT;
    mHeight = MATCH_PARENT;
    mX = 0;
    mY = 0;
    mType = TYPE_APPLICATION;
    mFlags = 0;
    mFormat = FORMAT_ARGB_8888;
    mToken = NULL;
    mInputFeatures = 0;
}

LayoutParams::~LayoutParams() {}

LayoutParams::LayoutParams(const LayoutParams& other)
      : mWidth(other.mWidth),
        mHeight(other.mHeight),
        mX(other.mX),
        mY(other.mY),
        mType(other.mType),
        mFlags(other.mFlags),
        mFormat(other.mFormat),
        mToken(other.mToken),
        mInputFeatures(other.mInputFeatures) {}

LayoutParams& LayoutParams::operator=(const LayoutParams& other) {
    if (this != &other) {
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mX = other.mX;
        mY = other.mY;
        mType = other.mType;
        mFlags = other.mFlags;
        mFormat = other.mFormat;
        mToken = other.mToken;
        mInputFeatures = other.mInputFeatures;
    }
    return *this;
}

status_t LayoutParams::writeToParcel(Parcel* out) const {
    SAFE_PARCEL(out->writeInt32, mWidth);
    SAFE_PARCEL(out->writeInt32, mHeight);
    SAFE_PARCEL(out->writeInt32, mX);
    SAFE_PARCEL(out->writeInt32, mY);
    SAFE_PARCEL(out->writeInt32, mType);
    SAFE_PARCEL(out->writeInt32, mFlags);
    SAFE_PARCEL(out->writeInt32, mFormat);
    SAFE_PARCEL(out->writeStrongBinder, mToken);
    SAFE_PARCEL(out->writeByte, mInputFeatures);
    return android::OK;
}

status_t LayoutParams::readFromParcel(const Parcel* in) {
    SAFE_PARCEL(in->readInt32, &mWidth);
    SAFE_PARCEL(in->readInt32, &mHeight);
    SAFE_PARCEL(in->readInt32, &mX);
    SAFE_PARCEL(in->readInt32, &mY);
    SAFE_PARCEL(in->readInt32, &mType);
    SAFE_PARCEL(in->readInt32, &mFlags);
    SAFE_PARCEL(in->readInt32, &mFormat);
    SAFE_PARCEL(in->readStrongBinder, &mToken);
    SAFE_PARCEL(in->readByte, &mInputFeatures);
    return android::OK;
}

} // namespace wm
} // namespace os
