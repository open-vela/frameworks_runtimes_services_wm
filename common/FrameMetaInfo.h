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
#include <optional>

#include "WindowUtils.h"

namespace os {
namespace wm {

// static constexpr size_t FRAME_META_INFO_SIZE = 8;
static constexpr int64_t FRAME_META_INVALID_VSYNC_ID = -1;

enum class FrameMetaIndex {
    Flags = 0,
    VsyncId,
    Vsync,

    FrameStart,
    LayoutStart,
    RenderStart,
    FrameInterval,
    RenderEnd,
    // End of frame meta info for UI proxy

    // for XMS arch
    SyncQueued,
    FrameFinished,
    NumIndexes
};

enum FrameMetaInfoFlags {
    SurfaceDraw = 1 << 0,
    SkipFrame = 1 << 1,
};

enum class FrameMetaSkipReason {
    NoTarget,
    NoSurface,
    NothingToDraw,
    NoBuffer,
};

class FrameMetaInfo {
public:
    FrameMetaInfo() {
        memset(mMetaData, 0, sizeof(mMetaData));
        mSkipReason = std::nullopt;
        set(FrameMetaIndex::VsyncId) = FRAME_META_INVALID_VSYNC_ID;
    }

    /* first usage */
    void setVsync(int64_t vsyncTime, int64_t vsyncId, int64_t frameIntervalMs) {
        memset(mMetaData, 0, sizeof(mMetaData));
        mSkipReason = std::nullopt;
        addFlag(FrameMetaInfoFlags::SurfaceDraw);

        set(FrameMetaIndex::VsyncId) = vsyncId;
        set(FrameMetaIndex::Vsync) = vsyncTime;
        set(FrameMetaIndex::FrameStart) = vsyncTime;
        set(FrameMetaIndex::LayoutStart) = vsyncTime;
        set(FrameMetaIndex::RenderStart) = vsyncTime;
        set(FrameMetaIndex::FrameInterval) = frameIntervalMs;
    }

    const int64_t* data() const {
        return mMetaData;
    }

    inline int64_t& set(FrameMetaIndex index) {
        return mMetaData[static_cast<int>(index)];
    }
    inline int64_t get(FrameMetaIndex index) const {
        if (index == FrameMetaIndex::NumIndexes) return 0;
        return mMetaData[static_cast<int>(index)];
    }
    inline int64_t operator[](FrameMetaIndex index) const {
        return get(index);
    }

    inline int64_t operator[](int index) const {
        if (index < 0 || index >= static_cast<int>(FrameMetaIndex ::NumIndexes)) return 0;
        return mMetaData[index];
    }
    inline int64_t getFrameInterval() const {
        return get(FrameMetaIndex::FrameInterval);
    }
    inline int64_t getVsyncId() const {
        return get(FrameMetaIndex::VsyncId);
    }
    inline int64_t duration(FrameMetaIndex start, FrameMetaIndex end) const {
        int64_t endTime = get(end);
        int64_t startTime = get(start);
        int64_t gap = endTime - startTime;
        gap = startTime > 0 ? gap : 0;
        /* TODO: */
        return gap > 0 ? gap : 0;
    }

    inline int64_t totalDuration() const {
        return duration(FrameMetaIndex::Vsync, FrameMetaIndex::FrameFinished);
    }

    inline int64_t totalTransactDuration() const {
        return duration(FrameMetaIndex::RenderEnd, FrameMetaIndex::FrameFinished);
    }

    inline int64_t totalVsyncDuration() const {
        return duration(FrameMetaIndex::Vsync, FrameMetaIndex::FrameStart);
    }

    inline int64_t totalDrawnDuration() const {
        return duration(FrameMetaIndex::Vsync, FrameMetaIndex::RenderEnd);
    }

    inline int64_t totalRenderDuration() const {
        return duration(FrameMetaIndex::RenderStart, FrameMetaIndex::RenderEnd);
    }

    inline int64_t totalLayoutDuration() const {
        return duration(FrameMetaIndex::LayoutStart, FrameMetaIndex::RenderStart);
    }

    void addFlag(int flag) {
        set(FrameMetaIndex::Flags) |= static_cast<uint64_t>(flag);
    }
    void markFrameStart() {
        set(FrameMetaIndex::FrameStart) = curSysTimeMs();
    }
    void markLayoutStart() {
        set(FrameMetaIndex::LayoutStart) = curSysTimeMs();
    }
    void markRenderStart() {
        set(FrameMetaIndex::RenderStart) = curSysTimeMs();
    }
    void markRenderEnd() {
        set(FrameMetaIndex::RenderEnd) = curSysTimeMs();
    }
    void markSyncQueued() {
        set(FrameMetaIndex::SyncQueued) = curSysTimeMs();
    }
    void markFrameFinished() {
        set(FrameMetaIndex::FrameFinished) = curSysTimeMs();
    }
    void setSkipReason(FrameMetaSkipReason reason) {
        addFlag(FrameMetaInfoFlags::SkipFrame);
        mSkipReason = reason;
    }
    inline std::optional<FrameMetaSkipReason> getSkipReason() const {
        return mSkipReason;
    }

    static inline int64_t getCurSysTime() {
        return curSysTimeMs();
    }

private:
    int64_t mMetaData[static_cast<int>(FrameMetaIndex::NumIndexes)];
    std::optional<FrameMetaSkipReason> mSkipReason;
};

} // namespace wm
} // namespace os
