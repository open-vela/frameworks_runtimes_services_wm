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
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/Log.h>

#include <vector>

#include "wm/BufferQueue.h"
#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

class BufferQueueTest : public ::testing::Test {
protected:
#ifndef CONFIG_SYSTEM_SERVER_LITE
    void SetUp() override {
        BufferId id1, id2;
        id1.mName = "buffer1";
        id1.mKey = 1;
        id1.mFd = -1;
        id2.mName = "buffer2";
        id2.mKey = 2;
        id2.mFd = -1;
        mVectorIds.push_back(id1);
        mVectorIds.push_back(id2);

        sp<IBinder> token = sp<android::BBinder>::make();
        sp<IBinder> handle = sp<android::BBinder>::make();
        int format = 0x10; // ARGB_8888
        int width = 200, height = 200;
        int size = width * height * 4;

        /* for server */
        mSCConsumer = std::make_shared<SurfaceControl>(token, handle, width, height, format, size);
        mSCConsumer->initBufferIds(mVectorIds);
        initSurfaceBuffer(mSCConsumer, true);
        mBuffConsumer = std::make_shared<BufferConsumer>(mSCConsumer);
        mSCConsumer->setBufferQueue(mBuffConsumer);

        /* for app */
        mSCProducer = std::make_shared<SurfaceControl>(token, handle, width, height, format, size);
        mSCProducer->initBufferIds(mVectorIds);
        initSurfaceBuffer(mSCProducer, false);
        mBuffProducer = std::make_shared<BufferProducer>(mSCProducer);
        mSCProducer->setBufferQueue(mBuffProducer);
    }
#endif

    void TearDown() override {}

    std::vector<BufferId> mVectorIds;

    std::shared_ptr<SurfaceControl> mSCConsumer;
    std::shared_ptr<BufferConsumer> mBuffConsumer;

    std::shared_ptr<SurfaceControl> mSCProducer;
    std::shared_ptr<BufferProducer> mBuffProducer;

    std::string mTestData = "Hello, world!";
}; // namespace os

#ifndef CONFIG_SYSTEM_SERVER_LITE
TEST_F(BufferQueueTest, ProducerConsumerTest) {
    EXPECT_NE(mBuffConsumer.get(), nullptr);
    EXPECT_NE(mBuffProducer.get(), nullptr);

    BufferItem* buffer = mBuffProducer->dequeueBuffer();
    EXPECT_NE(buffer, nullptr);

    memcpy(buffer->mBuffer, mTestData.c_str(), strlen(mTestData.c_str()) + 1);
    mBuffProducer->queueBuffer(buffer);

    EXPECT_NE(mBuffConsumer->syncQueuedState(buffer->mKey), nullptr);
    BufferItem* buffer2 = mBuffConsumer->acquireBuffer();
    EXPECT_NE(buffer2, nullptr);

    char* result = static_cast<char*>(buffer2->mBuffer);
    EXPECT_STREQ(result, mTestData.c_str());

    EXPECT_NE(mBuffProducer->syncFreeState(buffer2->mKey), nullptr);
    EXPECT_EQ(mBuffConsumer->releaseBuffer(buffer2), true);
}
#endif

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os
