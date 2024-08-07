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

#include <gtest/gtest.h>

#include "../common/FrameMetaInfo.h"

namespace os {
namespace wm {

class FrameMetaInfoTest : public ::testing::Test {
protected:
    FrameMetaInfo frameMetaInfo;

    void SetUp() override {
        // Set up code before each test case
    }

    void TearDown() override {
        // Clean up code after each test case
    }
};

TEST_F(FrameMetaInfoTest, TestConstructor) {
    EXPECT_EQ(frameMetaInfo.getVsyncId(), FRAME_META_INVALID_VSYNC_ID);
    EXPECT_EQ(frameMetaInfo.getFrameInterval(), 0);
    EXPECT_FALSE(frameMetaInfo.getSkipReason().has_value());
}

TEST_F(FrameMetaInfoTest, TestSetVsync) {
    int64_t vsyncTime = 123456789;
    int64_t vsyncId = 100;
    int64_t frameIntervalMs = 33;

    frameMetaInfo.setVsync(vsyncTime, vsyncId, frameIntervalMs);
    frameMetaInfo.setSkipReason(FrameMetaSkipReason::NoSurface);

    EXPECT_EQ(frameMetaInfo.getVsyncId(), vsyncId);
    EXPECT_EQ(frameMetaInfo.getFrameInterval(), frameIntervalMs);
    EXPECT_TRUE(frameMetaInfo.getSkipReason().has_value());
}

TEST_F(FrameMetaInfoTest, TestRenderDuration) {
    int64_t vsyncTime = curSysTimeMs();
    int64_t sleep_ms = 5;
    frameMetaInfo.setVsync(vsyncTime, 201, 33);

    sleep(sleep_ms);
    frameMetaInfo.markRenderEnd();

    int64_t duration = frameMetaInfo.totalRenderDuration();
    EXPECT_GE(duration, sleep_ms);
}

extern "C" int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os
