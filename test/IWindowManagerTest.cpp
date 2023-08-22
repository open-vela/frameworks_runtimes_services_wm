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

#include "BaseWindow.h"
#include "WindowManager.h"
#include "app/Application.h"
#include "app/ApplicationThread.h"
#include "app/Context.h"
#include "app/ContextImpl.h"
#include "app/UvLoop.h"

namespace os {
namespace wm {

class DemoApplication : public ::os::app::Application {
    void onCreate() override {}
    void onForeground() override {}
    void onBackground() override {}
    void onDestroy() override {}
};

class IWindowManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mWindowManager = WindowManager::getInstance();
        mToken = new BBinder();
        mApplication = new DemoApplication();

        mLooper = ::os::app::UvLoop();
        mApplication->setMainLoop(&mLooper);
        mContext = new ::os::app::ContextImpl(mApplication, mToken);

        mLayoutParam = LayoutParams();
        mLayoutParam.mToken = mToken;

        mWindow = mWindowManager->newWindow(mContext);
        mWindow->setLayoutParams(mLayoutParam);
        mWindow->setWindowManager(mWindowManager);
    }
    void TearDown() override {}

    WindowManager* mWindowManager;
    ::os::app::Context* mContext;
    std::shared_ptr<BaseWindow> mWindow;
    DemoApplication* mApplication;
    sp<IBinder> mToken;
    LayoutParams mLayoutParam;
    ::os::app::UvLoop mLooper;
};

TEST_F(IWindowManagerTest, GetPhysicalDisplayInfo) {
    Status status = Status::ok();
    int result = 0;
    DisplayInfo dispalyInfo;
    status = mWindowManager->getService()->getPhysicalDisplayInfo(1, &dispalyInfo, &result);
    EXPECT_TRUE(status.isOk());
}
TEST_F(IWindowManagerTest, AddWindow) {
    Status status = Status::ok();
    int result = 0;
    sp<IWindow> w = mWindow->getIWindow();
    LayoutParams lp = mWindow->getLayoutParams();
    InputChannel* outInputChannel = nullptr;
    outInputChannel = new InputChannel();

    status = mWindowManager->getService()->addWindowToken(mToken, 1, 1);
    status = mWindowManager->getService()->addWindow(w, lp, 1, 0, 1, outInputChannel, &result);
    EXPECT_TRUE(status.isOk());
}
TEST_F(IWindowManagerTest, RemoveWindow) {
    Status status = Status::ok();
    int result = 0;
    sp<IWindow> w = mWindow->getIWindow();
    LayoutParams lp = mWindow->getLayoutParams();
    InputChannel* outInputChannel = nullptr;
    outInputChannel = new InputChannel();

    status = mWindowManager->getService()->addWindowToken(mToken, 1, 1);
    EXPECT_TRUE(status.isOk());
    status = mWindowManager->getService()->addWindow(w, lp, 1, 0, 1, outInputChannel, &result);
    EXPECT_TRUE(status.isOk());
    status = mWindowManager->getService()->removeWindow(mWindow->getIWindow());
    EXPECT_TRUE(status.isOk());
}
TEST_F(IWindowManagerTest, Relayout) {
    // TODO
}
TEST_F(IWindowManagerTest, IsWindowToken) {
    // TODO
}
TEST_F(IWindowManagerTest, AddWindowToken) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, 1, 1);
    EXPECT_TRUE(status.isOk());
}

TEST_F(IWindowManagerTest, RemoveWindowToken) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, 1, 1);
    EXPECT_TRUE(status.isOk());
    status = mWindowManager->getService()->removeWindowToken(mToken, 1);
    EXPECT_TRUE(status.isOk());
}
TEST_F(IWindowManagerTest, UpdateWindowTokenVisibility) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, 1, 1);
    EXPECT_TRUE(status.isOk());
    status = mWindowManager->getService()->updateWindowTokenVisibility(mToken, 1);
    EXPECT_TRUE(status.isOk());
    status = mWindowManager->getService()->updateWindowTokenVisibility(mToken, 0);
    EXPECT_TRUE(status.isOk());
}

TEST_F(IWindowManagerTest, ApplyTransaction) {
    // TODO
}
TEST_F(IWindowManagerTest, RequestVsync) {
    // TODO
}

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace wm
} // namespace os