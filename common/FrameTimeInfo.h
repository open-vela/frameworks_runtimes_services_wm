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
#include "FrameMetaInfo.h"

namespace os {
namespace wm {

class FrameTimeInfo {
public:
    FrameTimeInfo();
    void time(FrameMetaInfo *info);

private:
    void init();
    void logPerSecond(bool checksec = true);

    int64_t mMinFrameTime;
    int64_t mMaxFrameTime;
    int64_t mTotalFrameTime;
    int64_t mLastLogFrameTime;
    int64_t mLastFrameFinishedTime;
    int64_t mFrameInterval;

    uint16_t mValidFrameSamples;
    uint16_t mTimeoutFrameSamples;
    uint16_t mSkipFrameSamples;
    uint16_t mSkipEmptyFrameSamples;
};

} // namespace wm
} // namespace os
