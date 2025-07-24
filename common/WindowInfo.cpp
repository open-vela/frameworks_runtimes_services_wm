/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#define LOG_TAG "WindowInfo"

#include "wm/WindowInfo.h"

#include "ParcelUtils.h"

namespace os {
namespace wm {

WindowInfo::WindowInfo() {
    mRoot = 0;
}

WindowInfo::~WindowInfo() {}

WindowInfo::WindowInfo(const WindowInfo& other) : mRoot(other.mRoot) {}

WindowInfo& WindowInfo::operator=(const WindowInfo& other) {
    if (this != &other) {
        mRoot = other.mRoot;
    }
    return *this;
}

status_t WindowInfo::writeToParcel(Parcel* out) const {
    SAFE_PARCEL(out->writeInt64, mRoot);
    return android::OK;
}

status_t WindowInfo::readFromParcel(const Parcel* in) {
    SAFE_PARCEL(in->readInt64, &mRoot);
    return android::OK;
}

} // namespace wm
} // namespace os
