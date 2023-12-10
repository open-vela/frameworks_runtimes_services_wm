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
    static const int32_t WINDOW_INVISIBLE = 0;
    static const int32_t WINDOW_VISIBLE = 1;
    static const int32_t WINDOW_GONE = 2;

    // for type
    static const int32_t TYPE_APPLICATION = 1;
    static const int32_t TYPE_SYSTEM_WINDOW = 1000;
    static const int32_t TYPE_TOAST = TYPE_SYSTEM_WINDOW + 1;
    static const int32_t TYPE_DIALOG = TYPE_SYSTEM_WINDOW + 2;
    static const int32_t INVALID_WINDOW_TYPE = -1;

    static const int32_t MATCH_PARENT = -1;

    // for format
    static const int32_t FORMAT_UNKNOWN = 0;
    static const int32_t FORMAT_TRANSPARENT = -2;
    static const int32_t FORMAT_OPAQUE = -1;
    static const int32_t FORMAT_RGB_565 = 0x12;
    static const int32_t FORMAT_RGB_565A8 = 0x14;
    static const int32_t FORMAT_RGB_888 = 0x0F;
    static const int32_t FORMAT_ARGB_8888 = 0x10;
    static const int32_t FORMAT_XRGB_8888 = 0x11;

    LayoutParams();
    ~LayoutParams();

    LayoutParams(const LayoutParams& other);
    LayoutParams& operator=(const LayoutParams& other);

    status_t writeToParcel(Parcel* out) const override;
    status_t readFromParcel(const Parcel* in) override;

    bool hasInput() const {
        return (mInputFeatures & INPUT_FEATURE_NO_INPUT_CHANNEL) == 0;
    }

    int32_t mWidth;
    int32_t mHeight;
    int32_t mX;
    int32_t mY;
    int32_t mType;
    int32_t mFlags;
    int32_t mFormat;
    sp<IBinder> mToken;

private:
    /**
     * Does not construct an input channel for this window.  The channel will therefore
     * be incapable of receiving input.
     */
    static const int INPUT_FEATURE_NO_INPUT_CHANNEL = 1 << 0;

    /**
     * Control a set of features of the input subsystem that are exposed to the app process.
     */
    int8_t mInputFeatures;
};

} // namespace wm
} // namespace os
