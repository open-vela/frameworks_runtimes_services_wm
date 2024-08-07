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

#define LOG_TAG "FrameTime"

#include "FrameTimeInfo.h"

namespace os {
namespace wm {

FrameTimeInfo::FrameTimeInfo() {
    init();
}

void FrameTimeInfo::time(FrameMetaInfo* info) {
    if (!info) {
        logPerSecond();
        return;
    }

    auto skipReason = info->getSkipReason();
    if (skipReason) {
        if (skipReason == FrameMetaSkipReason::NothingToDraw) mSkipEmptyFrameSamples++;
        mSkipFrameSamples++;
        /* discard invalid samples */
        if (mSkipFrameSamples > 255) {
            mSkipEmptyFrameSamples = mSkipFrameSamples = 0;
        }
        return;
    }

    /* first valid frame */
    if (mLastFrameFinishedTime == 0) {
        mSkipEmptyFrameSamples = mSkipFrameSamples = mTimeoutFrameSamples = 0;
        mTotalFrameTime = mMinFrameTime = mMaxFrameTime = 0;
        mLastLogFrameTime = info->get(FrameMetaIndex::Vsync);
    }

    mValidFrameSamples++;
    auto curFrameTime = info->totalDuration();
    mTotalFrameTime += curFrameTime;

    mMaxFrameTime = fmax(mMaxFrameTime, curFrameTime);
    if (mMinFrameTime == 0)
        mMinFrameTime = curFrameTime;
    else
        mMinFrameTime = fmin(mMinFrameTime, curFrameTime);

    mFrameInterval = info->getFrameInterval();
    if (mFrameInterval > 0 && curFrameTime > mFrameInterval) mTimeoutFrameSamples++;

    mLastFrameFinishedTime = info->get(FrameMetaIndex::FrameFinished);
    logPerSecond();
}

void FrameTimeInfo::logPerSecond() {
    if (mLastFrameFinishedTime == 0) return;

    auto nowTime = FrameMetaInfo::getCurSysTime();
    auto interval = mLastFrameFinishedTime - mLastLogFrameTime;
    /* 1s */
    if (nowTime - mLastLogFrameTime + mFrameInterval > 1000) {
        FLOGW("FrameLog{ timeMs=%" PRId64 "(%" PRId64 "), frames=%" PRIu16 ":%" PRIu16 "(%" PRIu16
              ":%" PRIu16 ":%" PRIu16 "), minMs=%" PRId64 ", maxMs=%" PRId64
              "ms, avgMs=%.2f, intervalMs=%" PRId64 ", skip=(%" PRIu16 "/%" PRIu16 ") }",
              mTotalFrameTime, interval, mValidFrameSamples,
              mValidFrameSamples + mSkipEmptyFrameSamples,
              mValidFrameSamples - mTimeoutFrameSamples, mTimeoutFrameSamples,
              mSkipEmptyFrameSamples, mMinFrameTime, mMaxFrameTime,
              mTotalFrameTime * 1. / mValidFrameSamples, mFrameInterval, mSkipEmptyFrameSamples,
              mSkipFrameSamples);
        init();
    }
}

void FrameTimeInfo::init() {
    mValidFrameSamples = mTimeoutFrameSamples = 0;
    mSkipFrameSamples = mSkipEmptyFrameSamples = 0;
    mFrameInterval = 0;
    mLastFrameFinishedTime = mLastLogFrameTime = 0;
    mTotalFrameTime = mMinFrameTime = mMaxFrameTime = 0;
}

} // namespace wm
} // namespace os
