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

#include "wm/FakeFmq.h"

using namespace os::wm;

class FakeFmqTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::string name = "/var/shm/xms:test-fmq-" + std::to_string(std::rand());
        mServerFmq.setName(name);
        mServerFmq.create(mData, true);

        Parcel parcel;
        mServerFmq.writeToParcel(&parcel);
        parcel.setDataPosition(0);

        mClientFmq.readFromParcel(&parcel);
        mClientFmq.create(mData, false);
    }

    void TearDown() override {
        mServerFmq.destroy();
        mClientFmq.destroy();
    }

    std::vector<int> mData = {1, 2};
    FakeFmq<int> mServerFmq;
    FakeFmq<int> mClientFmq;

    int mExpectedOne = 1;
    int mExpectedTwo = 2;
};

TEST_F(FakeFmqTest, TestFmqName) {
    ASSERT_EQ(mServerFmq.getName(), mClientFmq.getName());
}

TEST_F(FakeFmqTest, TestClientRead) {
    int result = 0;

    ASSERT_TRUE(mClientFmq.read(&result));
    ASSERT_EQ(result, mExpectedOne);

    ASSERT_TRUE(mClientFmq.read(&result));
    ASSERT_EQ(result, mExpectedTwo);
}

TEST_F(FakeFmqTest, TestServerWriteClientRead) {
    int result = 0;

    ASSERT_TRUE(mClientFmq.read(&result));
    ASSERT_TRUE(mClientFmq.read(&result));

    ASSERT_TRUE(mServerFmq.write(&mExpectedOne));
    ASSERT_TRUE(mClientFmq.read(&result));
    ASSERT_EQ(result, mExpectedOne);

    ASSERT_TRUE(mServerFmq.write(&mExpectedTwo));
    ASSERT_TRUE(mClientFmq.read(&result));
    ASSERT_EQ(result, mExpectedTwo);
}

extern "C" int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
