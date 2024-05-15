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

#define GESTURE_DETECTOR_TRIGGER_DISTANCE 13
#define GESTURE_DETECTOR_INVALID_DISTANCE 57
#define GESTURE_SCREEN_STATUS_KVDB_KEY "tmp.es.screen_status"

namespace os::wm {

constexpr uint8_t swipe_up = 1 << 0;
constexpr uint8_t swipe_down = 1 << 1;
constexpr uint8_t swipe_left = 1 << 2;
constexpr uint8_t swipe_right = 1 << 3;
constexpr uint8_t trigger_x = 1 << 4;
constexpr uint8_t trigger_y = 1 << 5;
constexpr uint8_t screen_off = 1 << 6;

inline bool is_x_swipe(uint8_t swipe) {
    return swipe & (swipe_left | swipe_right);
};
inline bool is_y_swipe(uint8_t swipe) {
    return swipe & (swipe_up | swipe_down);
};
inline bool is_swipe_left(uint8_t swipe) {
    return swipe & swipe_left;
};
inline bool is_swipe_right(uint8_t swipe) {
    return swipe & swipe_right;
};
inline bool is_swipe_up(uint8_t swipe) {
    return swipe & swipe_up;
};
inline bool is_swipe_down(uint8_t swipe) {
    return swipe & swipe_down;
};
inline bool is_trigger_x(uint8_t swipe) {
    return swipe & trigger_x;
};
inline bool is_trigger_y(uint8_t swipe) {
    return swipe & trigger_y;
};
inline bool is_screen_off(uint8_t swipe) {
    return swipe & screen_off;
}

} // namespace os::wm
