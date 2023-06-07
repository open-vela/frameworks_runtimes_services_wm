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

#define LOG_TAG "WindowFrames"

#include "wm/WindowFrames.h"

namespace os {
namespace wm {

WindowFrames::WindowFrames() {}
WindowFrames::~WindowFrames() {}

WindowFrames::WindowFrames(int32_t left, int32_t top, int32_t right, int32_t bottom)
      : mLeft(left), mTop(top), mRight(right), mBottom(bottom) {}

status_t WindowFrames::writeToParcel(Parcel* out) const {
    status_t result = out->writeInt32(mLeft);
    if (result != android::OK) return result;

    result = out->writeInt32(mTop);
    if (result != android::OK) return result;

    result = out->writeInt32(mRight);
    if (result != android::OK) return result;

    result = out->writeInt32(mBottom);
    return result;
}

status_t WindowFrames::readFromParcel(const Parcel* in) {
    status_t result = in->readInt32(&mLeft);
    if (result != android::OK) return result;

    result = in->readInt32(&mTop);
    if (result != android::OK) return result;

    result = in->readInt32(&mRight);
    if (result != android::OK) return result;

    result = in->readInt32(&mBottom);
    return result;
}

int32_t WindowFrames::getLeft() const {
    return mLeft;
}

int32_t WindowFrames::getTop() const {
    return mTop;
}

int32_t WindowFrames::getRight() const {
    return mRight;
}

int32_t WindowFrames::getBottom() const {
    return mBottom;
}

} // namespace wm
} // namespace os