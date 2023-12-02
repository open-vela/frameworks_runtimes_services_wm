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

static InputMessage gTestMessage = {
        .type = INPUT_MESSAGE_TYPE_POINTER,
        .state = INPUT_MESSAGE_STATE_PRESSED,
        .pointer = {.x = 100, .y = 100, .raw_x = 100, .raw_y = 100},
};

class InputMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        uv_loop_init(&mUVLooper);
        uv_run(&mUVLooper, UV_RUN_DEFAULT);
    }

    void TearDown() override {
        uv_loop_close(&mUVLooper);
    }

    uv_loop_t mUVLooper;
};

TEST_F(InputMonitorTest, monitorInput) {
    std::shared_ptr<InputMonitor> monitor = WindowManager::monitorInput("input-gesture-test1", 0);
    EXPECT_NE(monitor, nullptr);
    EXPECT_EQ(monitor->isValid(), true);
}

TEST_F(InputMonitorTest, receiveMessage) {
    const char* channel_name = "input-gesture-test2";

    /* create dispatcher for server */
    auto dispatcher = std::make_shared<InputDispatcher>();
    bool ret = dispatcher->create(channel_name);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(dispatcher->getInputChannel().isValid(), true);

    /* create monitor for client */
    InputChannel* channel = new InputChannel();
    channel->copyFrom(dispatcher->getInputChannel());

    sp<IBinder> token = new BBinder();
    auto monitor = std::make_shared<InputMonitor>(token, channel);
    EXPECT_EQ(monitor->isValid(), true);

    /* server sending message */
    ret = dispatcher->sendMessage(&gTestMessage);
    EXPECT_EQ(ret, true);

    /* client receiving message */
    InputMessage im;
    ret = monitor->receiveMessage(&im);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(im.type, gTestMessage.type);
    EXPECT_EQ(im.state, gTestMessage.state);
    EXPECT_EQ(im.pointer.x, gTestMessage.pointer.x);
    EXPECT_EQ(im.pointer.y, gTestMessage.pointer.y);
}

TEST_F(InputMonitorTest, start) {
    auto input = WindowManager::monitorInput("input-gesture-test3", 0);
    input->start(&mUVLooper, [](InputMonitor* monitor) {
        if (!monitor || !monitor->isValid()) return;
        InputMessage msg;
        bool ret = monitor->receiveMessage(&msg);
        EXPECT_EQ(ret, true);
    });
}

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os
