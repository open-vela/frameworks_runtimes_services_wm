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

#include <gtest/gtest.h>
#include <nuttx/config.h>
#include <uv.h>

#include "../server/InputDispatcher.h"
#include "WindowManager.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

class InputMonitorTest : public ::testing::Test {
#ifndef CONFIG_SYSTEM_SERVER_LITE
protected:
    void SetUp() override {
        uv_loop_init(&mUVLooper);
        uv_run(&mUVLooper, UV_RUN_DEFAULT);
    }

    void TearDown() override {
        uv_loop_close(&mUVLooper);
    }

    uv_loop_t mUVLooper;
#endif
};

TEST_F(InputMonitorTest, monitorInput) {
    std::shared_ptr<InputMonitor> monitor = WindowManager::monitorInput("input-gesture-test1", 0);
    EXPECT_NE(monitor, nullptr);
    EXPECT_EQ(monitor->isValid(), true);
}

#ifndef CONFIG_SYSTEM_SERVER_LITE
TEST_F(InputMonitorTest, start) {
    auto input = WindowManager::monitorInput("input-gesture-test3", 0);
    EXPECT_NE(input, nullptr);
    bool ret = input->start(&mUVLooper, [](InputMonitor* monitor) {
        if (!monitor || !monitor->isValid()) return;
        InputMessage msg;
        bool recv_ret = monitor->receiveMessage(&msg);
        EXPECT_EQ(recv_ret, true);
    });

    EXPECT_EQ(ret, true);
}
#endif

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os
