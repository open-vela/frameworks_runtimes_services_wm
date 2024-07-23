/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#include <kvdb.h>
#include <os/wm/DisplayInfo.h>

#include <algorithm>
#include <cstdint>
#include <memory>

#include "../common/WindowUtils.h"
#include "app/UvLoop.h"
#include "uv.h"
#include "wm/GestureDetectorState.h"
#include "wm/InputMessageBase.h"

namespace os::wm {

class GestureDetector {
public:
    GestureDetector() = delete;
    explicit GestureDetector(std::shared_ptr<::os::app::UvLoop> uvLoop)
          : mIsScreenOn(property_get_int32(GESTURE_SCREEN_STATUS_KVDB_KEY, 1) > 0),
            mUvLoop(uvLoop),
            mFD(property_monitor_open(GESTURE_SCREEN_STATUS_KVDB_KEY)) {
        if (mFD > 0) {
            mUvPoll = std::make_shared<::os::app::UvPoll>(mUvLoop->get(), mFD);
            mUvPoll->start(
                    UV_READABLE,
                    [](int fd, int status, int events, void* data) {
                        auto* gd = ((GestureDetector*)data);
                        char tmpKey[30];
                        char tmpVal[10];
                        property_monitor_read(gd->mFD, tmpKey, tmpVal);
                        gd->mIsScreenOn = strcmp(tmpVal, "-1") != 0;
                    },
                    this);
        } else {
            ALOGE("FATAL ERROR, mFD=%d\n", mFD);
        }
    }

    ~GestureDetector() {
        if (mFD && mUvPoll) {
            mUvPoll->stop();
            property_monitor_close(mFD);
            mUvPoll = nullptr;
        }
    }

    uint8_t recognizeGesture(const InputMessage* msg) {
        uint8_t ret = 0;
        int current_x = msg->pointer.x;
        int current_y = msg->pointer.y;

        if (msg->state == INPUT_MESSAGE_STATE_PRESSED) {
            if (!mIsScreenOn) {
                mSwipe |= screen_off;
                ret = mSwipe;
                goto out;
            }

            if (mLastX == current_x && mLastY == current_y) {
                return mSwipe;
            }
            if (mLastInputState == INPUT_MESSAGE_STATE_RELEASED) {
                mPressedX = current_x;
                mPressedY = current_y;
                int left = std::clamp(mPressedX, 0, 0 + GESTURE_DETECTOR_TRIGGER_DISTANCE);
                int top = std::clamp(mPressedY, 0, 0 + GESTURE_DETECTOR_TRIGGER_DISTANCE);
                int right = std::clamp(mPressedX, mW - GESTURE_DETECTOR_TRIGGER_DISTANCE, mW);
                int bottom = std::clamp(mPressedY, mH - GESTURE_DETECTOR_TRIGGER_DISTANCE, mH);

                if (top == mPressedY) {
                    mSwipe |= swipe_down;
                } else if (bottom == mPressedY) {
                    mSwipe |= swipe_up;
                } else if (left == mPressedX) {
                    mSwipe |= swipe_right;
                } else if (right == mPressedX) {
                    mSwipe |= swipe_left;
                }
                if (!is_x_swipe(mSwipe) && !is_y_swipe(mSwipe)) {
                    ret = 0;
                    goto out;
                }
            } else {
                if ((is_swipe_left(mSwipe) &&
                     (mPressedX - current_x >= GESTURE_DETECTOR_INVALID_DISTANCE)) ||
                    (is_swipe_right(mSwipe) &&
                     (current_x - mPressedX >= GESTURE_DETECTOR_INVALID_DISTANCE))) {
                    mSwipe |= trigger_x;
                } else if ((is_swipe_up(mSwipe) &&
                            (mPressedY - current_y >= GESTURE_DETECTOR_INVALID_DISTANCE)) ||
                           (is_swipe_down(mSwipe) &&
                            (current_y - mPressedY >= GESTURE_DETECTOR_INVALID_DISTANCE))) {
                    mSwipe |= trigger_y;
                } else {
                    mSwipe &= ~trigger_x;
                    mSwipe &= ~trigger_y;
                }
            }
            ret = mSwipe;
        } else if (msg->state == INPUT_MESSAGE_STATE_RELEASED) {
            if (!is_x_swipe(mSwipe) && !is_y_swipe(mSwipe) && !is_screen_off(mSwipe)) {
                goto out;
            }
            ret = mSwipe;
            mSwipe = 0;
        }
    out:
        mLastInputState = msg->state;
        mLastX = current_x;
        mLastY = current_y;
        return ret;
    }

    void setDisplayInfo(DisplayInfo* info) {
        mH = info->height;
        mW = info->width;
    }

private:
    int mH{};
    int mW{};
    bool mIsScreenOn{true};
    std::shared_ptr<::os::app::UvLoop> mUvLoop{};
    int mFD{};
    std::shared_ptr<::os::app::UvPoll> mUvPoll{};
    InputMessageState mLastInputState{INPUT_MESSAGE_STATE_RELEASED};
    uint8_t mSwipe{};
    int mPressedX{};
    int mPressedY{};
    int mLastX{};
    int mLastY{};
};

} // namespace os::wm
