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

#include <vector>

#include "wm/BufferQueue.h"
#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

class BufferQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        int fd1 = shm_open("testBuffer1", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        ftruncate(fd1, 20);

        int fd2 = shm_open("testBuffer2", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        ftruncate(fd2, 20);

        BufferId id1;
        id1.mName = "testBuffer1";
        id1.mKey = 1;
        id1.mFd = fd1;

        BufferId id2;
        id2.mName = "testBuffer2";
        id2.mKey = 2;
        id2.mFd = fd2;

        mIdsConsumer.push_back(id1);
        mIdsConsumer.push_back(id2);

        mIdsProducer.push_back(id1);
        mIdsProducer.push_back(id2);

        mSCConsumer = std::make_shared<SurfaceControl>();
        mSCConsumer->initBufferIds(mIdsConsumer);

        mSCProducer = std::make_shared<SurfaceControl>();
        mSCProducer->initBufferIds(mIdsProducer);
    } // namespace wm
    void TearDown() override {}

    std::vector<BufferId> mIdsConsumer;
    std::vector<BufferId> mIdsProducer;
    std::shared_ptr<SurfaceControl> mSCConsumer;
    std::shared_ptr<SurfaceControl> mSCProducer;
}; // namespace os

TEST_F(BufferQueueTest, CreateBufferQueue) {
    std::shared_ptr<BufferConsumer> buffConsumer = std::make_shared<BufferConsumer>(mSCConsumer);
    EXPECT_NE(buffConsumer.get(), nullptr);
}

TEST_F(BufferQueueTest, DequeueQueueBuffer) {
    std::shared_ptr<BufferProducer> buffProducer = std::make_shared<BufferProducer>(mSCConsumer);
    BufferItem* buffer = buffProducer->dequeueBuffer();
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffProducer->queueBuffer(buffer), true);
}

TEST_F(BufferQueueTest, AcquiredReleaseBuffer) {
    std::shared_ptr<BufferConsumer> buffConsumer = std::make_shared<BufferConsumer>(mSCConsumer);
    std::shared_ptr<BufferProducer> buffProducer = std::make_shared<BufferProducer>(mSCConsumer);
    std::string data = "Hello, world!";
    BufferItem* buffer = buffProducer->dequeueBuffer();
    memcpy(buffer->mBuffer, data.c_str(), strlen(data.c_str()) + 1);

    buffProducer->queueBuffer(buffer);
    EXPECT_NE(buffConsumer->syncQueuedState(buffer->mKey), nullptr);
    BufferItem* buffer2 = buffConsumer->acquireBuffer();
    EXPECT_NE(buffer2, nullptr);

    char* result = static_cast<char*>(buffer2->mBuffer);
    EXPECT_STREQ(result, data.c_str());

    EXPECT_NE(buffProducer->syncFreeState(buffer2->mKey), nullptr);
    EXPECT_EQ(buffConsumer->releaseBuffer(buffer2), true);
}

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace wm
} // namespace os