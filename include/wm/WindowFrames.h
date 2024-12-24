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

#include "wm/Rect.h"

namespace os {
namespace wm {

using namespace android;
using namespace android::base;
using namespace android::binder;

/**
 * @class WindowFrames
 * @brief Represents the frames for a window.
 *
 * This class encapsulates frame information for a window, including
 * its dimensions and related attributes. It is used by the window
 * manager to manage window rendering and layout.
 */
class WindowFrames : public Parcelable {
public:
    WindowFrames();

    ~WindowFrames();

    WindowFrames(const Rect& rect);

    /**
     * @brief Writes the WindowFrames data to a Parcel.
     *
     * This method overrides the writeToParcel method from the Parcelable
     * interface.
     *
     * @param out Pointer to the Parcel where data will be written.
     * @return Status indicating the success or failure of the operation.
     */
    status_t writeToParcel(Parcel* out) const override;

    /**
     * @brief Reads the WindowFrames data from a Parcel.
     *
     * This method overrides the readFromParcel method from the Parcelable
     * interface.
     *
     * @param in Pointer to the Parcel from which data will be read.
     * @return Status indicating the success or failure of the operation.
     */
    status_t readFromParcel(const Parcel* in) override;

    /**
     * @brief Retrieves the actual window bounds.
     *
     * @return The Rect object representing the window bounds.
     */
    Rect getFrame() const {
        return mFrame;
    }

private:
    Rect mFrame;
};

} // namespace wm
} // namespace os
