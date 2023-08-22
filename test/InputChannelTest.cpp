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
#include <mqueue.h>

#include <vector>

#include "app/UvLoop.h"
#include "wm/InputChannel.h"
#include "wm/InputMessage.h"

namespace os {
namespace wm {

class InputChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        mName = "InputChannelTest";
        mInputChannel = std::make_shared<InputChannel>();
        mInputChannel2 = std::make_shared<InputChannel>();

        mInputMsg.type = INPUT_MESSAGE_TYPE_POINTER;
        mInputMsg.state = INPUT_MESSAGE_STATE_PRESSED;
        mInputMsg.keypad.key_code = 111;
        mInputMsg.pointer.x = 100;
        mInputMsg.pointer.y = 100;
        mInputMsg.pointer.raw_x = 200;
        mInputMsg.pointer.raw_y = 200;

    } // namespace wm
    void TearDown() override {}

    std::shared_ptr<InputChannel> mInputChannel;
    std::shared_ptr<InputChannel> mInputChannel2;

    std::string mName;
    InputMessage mInputMsg;

}; // namespace wm

TEST_F(InputChannelTest, CreateInputChannel) {
    EXPECT_EQ(mInputChannel->create(mName), true);
}

TEST_F(InputChannelTest, FdValidTest) {
    EXPECT_EQ(mInputChannel->create(mName), true);

    mInputChannel2->copyFrom(*mInputChannel);
    EXPECT_EQ(mInputChannel2->isValid(), true);
}

TEST_F(InputChannelTest, SendMessage) {
    EXPECT_EQ(mInputChannel->create(mName), true);

    ::os::app::UvLoop looper;
    ::os::app::UvLoop* handler = &looper;
    ::os::app::UvPoll pollfd(looper, mInputChannel->getEventFd());
    pollfd.start(
            UV_READABLE,
            [handler](int f, int status, int events, void* data) {
                InputMessage msg;
                mq_receive(f, (char*)&msg, sizeof(InputMessage), NULL);

                InputMessage* ie = &msg;
                if (ie->type == INPUT_MESSAGE_TYPE_POINTER) {
                    EXPECT_EQ(ie->pointer.x, 100);
                    EXPECT_EQ(ie->pointer.y, 100);
                    EXPECT_EQ(ie->pointer.raw_x, 200);
                    EXPECT_EQ(ie->pointer.raw_y, 200);
                } else if (ie->type == INPUT_MESSAGE_TYPE_KEYPAD) {
                    EXPECT_EQ(ie->keypad.key_code, (uint32_t)111);
                }

                handler->stop();
            },
            nullptr);

    EXPECT_EQ(mInputChannel->sendMessage(&mInputMsg), true);
    looper.run();
}

TEST_F(InputChannelTest, ReleaseInputChannel) {
    EXPECT_EQ(mInputChannel->create(mName), true);

    mInputChannel->release();
    EXPECT_EQ(mInputChannel->isValid(), false);
}

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os