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
#include <cstdint>

// Constants for gesture detection thresholds and keys
#define GESTURE_DETECTOR_TRIGGER_DISTANCE 13
#define GESTURE_DETECTOR_INVALID_DISTANCE 57
#define GESTURE_SCREEN_STATUS_KVDB_KEY "persist.brightness.target"

namespace os::wm {

/**
 * @brief Constants representing different swipe directions and triggers.
 */
constexpr uint8_t swipe_up = 1 << 0;
constexpr uint8_t swipe_down = 1 << 1;
constexpr uint8_t swipe_left = 1 << 2;
constexpr uint8_t swipe_right = 1 << 3;
constexpr uint8_t trigger_x = 1 << 4;
constexpr uint8_t trigger_y = 1 << 5;
constexpr uint8_t screen_off = 1 << 6;

/**
 * @brief Checks if the swipe gesture is in the X direction.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is an X-direction swipe (left or right), false otherwise.
 */
inline bool is_x_swipe(uint8_t swipe) {
    return swipe & (swipe_left | swipe_right);
}

/**
 * @brief Checks if the swipe gesture is in the Y direction.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is a Y-direction swipe (up or down), false otherwise.
 */
inline bool is_y_swipe(uint8_t swipe) {
    return swipe & (swipe_up | swipe_down);
}

/**
 * @brief Checks if the swipe gesture is a left swipe.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is a left swipe, false otherwise.
 */
inline bool is_swipe_left(uint8_t swipe) {
    return swipe & swipe_left;
}

/**
 * @brief Checks if the swipe gesture is a right swipe.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is a right swipe, false otherwise.
 */
inline bool is_swipe_right(uint8_t swipe) {
    return swipe & swipe_right;
}

/**
 * @brief Checks if the swipe gesture is an upward swipe.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is an upward swipe, false otherwise.
 */
inline bool is_swipe_up(uint8_t swipe) {
    return swipe & swipe_up;
}

/**
 * @brief Checks if the swipe gesture is a downward swipe.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if it is a downward swipe, false otherwise.
 */
inline bool is_swipe_down(uint8_t swipe) {
    return swipe & swipe_down;
}

/**
 * @brief Checks if the gesture triggered on the X axis.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if the trigger is on the X axis, false otherwise.
 */
inline bool is_trigger_x(uint8_t swipe) {
    return swipe & trigger_x;
}

/**
 * @brief Checks if the gesture triggered on the Y axis.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if the trigger is on the Y axis, false otherwise.
 */
inline bool is_trigger_y(uint8_t swipe) {
    return swipe & trigger_y;
}

/**
 * @brief Checks if the screen is turned off based on the swipe gesture.
 *
 * @param swipe The swipe gesture represented by a bitmask.
 * @return True if the gesture indicates the screen is off, false otherwise.
 */
inline bool is_screen_off(uint8_t swipe) {
    return swipe & screen_off;
}

} // namespace os::wm
