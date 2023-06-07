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

class WindowFrames : public Parcelable {
public:
    WindowFrames();
    ~WindowFrames();

    WindowFrames(int32_t left, int32_t top, int32_t right, int32_t bottom);

    status_t writeToParcel(Parcel* out) const override;
    status_t readFromParcel(const Parcel* in) override;

    int32_t getLeft() const;
    int32_t getTop() const;
    int32_t getRight() const;
    int32_t getBottom() const;

private:
    int32_t mLeft;
    int32_t mTop;
    int32_t mRight;
    int32_t mBottom;
};

} // namespace wm
} // namespace os
