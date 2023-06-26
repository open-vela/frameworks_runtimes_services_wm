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

namespace os {
namespace wm {

using namespace android;
using namespace android::base;
using namespace android::binder;
using namespace std;

class LayoutParams : public Parcelable {
public:
    LayoutParams();
    ~LayoutParams();

    LayoutParams(int32_t width, int32_t height, int32_t x, int32_t y, int32_t type, int32_t flags,
                 int32_t format)
          : mWidth(width),
            mHeight(height),
            mX(x),
            mY(y),
            mType(type),
            mFlags(flags),
            mFormat(format) {}

    LayoutParams(const LayoutParams& other);
    LayoutParams& operator=(const LayoutParams& other);

    status_t writeToParcel(Parcel* out) const override;
    status_t readFromParcel(const Parcel* in) override;

    int32_t mWidth;
    int32_t mHeight;
    int32_t mX;
    int32_t mY;
    int32_t mType;
    int32_t mFlags;
    int32_t mFormat;
    sp<IBinder> mToken;

private:
};

} // namespace wm
} // namespace os
