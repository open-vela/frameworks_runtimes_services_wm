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

using namespace android;

WindowFrames::WindowFrames() {}
WindowFrames::~WindowFrames() {}

WindowFrames::WindowFrames(const Rect& rect) {
    mFrame = rect;
}

status_t WindowFrames::writeToParcel(Parcel* out) const {
    return mFrame.writeToParcel(out);
}

status_t WindowFrames::readFromParcel(const Parcel* in) {
    return mFrame.readFromParcel(in);
}

} // namespace wm
} // namespace os
