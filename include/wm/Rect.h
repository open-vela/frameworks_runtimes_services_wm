
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

#include "ParcelUtils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BaseRect {
    // Minimum X coordinate of the rectangle.
    int32_t left = 0;

    // Minimum Y coordinate of the rectangle.
    int32_t top = 0;

    // Maximum X coordinate of the rectangle.
    int32_t right = 0;

    // Maximum Y coordinate of the rectangle.
    int32_t bottom = 0;
} BaseRect;

#ifdef __cplusplus
};
#endif

namespace os {
namespace wm {

using android::BAD_VALUE;
using android::Parcel;
using android::status_t;

class Rect : public BaseRect {
public:
    Rect() {}
    ~Rect() {}

    Rect(int32_t l, int32_t t, int32_t r, int32_t b) {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
    Rect(const Rect& other) {
        left = other.left;
        top = other.top;
        right = other.right;
        bottom = other.bottom;
    }
    int32_t getLeft() const {
        return left;
    }

    int32_t getTop() const {
        return top;
    }

    int32_t getWidth() const {
        return right - left;
    }

    int32_t getHeight() const {
        return bottom - top;
    }

    Rect& operator=(const Rect& other) {
        if (this != &other) {
            left = other.left;
            top = other.top;
            right = other.right;
            bottom = other.bottom;
        }
        return *this;
    }

    status_t writeToParcel(Parcel* out) const {
        SAFE_PARCEL(out->writeInt32, left);
        SAFE_PARCEL(out->writeInt32, top);
        SAFE_PARCEL(out->writeInt32, right);
        SAFE_PARCEL(out->writeInt32, bottom);
        return android::OK;
    }

    status_t readFromParcel(const Parcel* in) {
        SAFE_PARCEL(in->readInt32, &left);
        SAFE_PARCEL(in->readInt32, &top);
        SAFE_PARCEL(in->readInt32, &right);
        SAFE_PARCEL(in->readInt32, &bottom);
        return android::OK;
    }
};
} // namespace wm
} // namespace os
