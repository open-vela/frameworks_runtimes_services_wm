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
        mWindowManager = WindowManager::create();
        mToken = new BBinder();
        mApplication = new DemoApplication();

        mLooper = new ::os::app::UvLoop(false);
        mApplication->setMainLoop(mLooper);
        mContext = new ::os::app::ContextImpl(mApplication, "test", mToken, mLooper);

        mLayoutParam = LayoutParams();
        mLayoutParam.mToken = mToken;
        mLayoutParam.enableInput(false);

        mWindow = mWindowManager->newWindow(mContext);
        mWindow->setLayoutParams(mLayoutParam);
    }

    void TearDown() override {
        mWindow = nullptr;
        delete mWindowManager;
    }

    WindowManager* mWindowManager;
    ::os::app::Context* mContext;
    std::shared_ptr<BaseWindow> mWindow;
    DemoApplication* mApplication;
    sp<IBinder> mToken;
    LayoutParams mLayoutParam;
    ::os::app::UvLoop* mLooper;
};

TEST_F(IWindowManagerTest, GetPhysicalDisplayInfo) {
    uint32_t width = 0, height = 0;
    mWindowManager->getDisplayInfo(&width, &height);
}

TEST_F(IWindowManagerTest, AddRemoveWindowToken) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, LayoutParams::TYPE_APPLICATION, 1,
                                                          "com.vela.test.windowtoken");
    EXPECT_TRUE(status.isOk());

    status = mWindowManager->getService()->removeWindowToken(mToken, 1);
    EXPECT_TRUE(status.isOk());
}

TEST_F(IWindowManagerTest, AddRemoveWindow) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, LayoutParams::TYPE_APPLICATION, 1,
                                                          "com.vela.test.window");
    EXPECT_TRUE(status.isOk());

    int32_t result = mWindowManager->attachIWindow(mWindow);
    EXPECT_EQ(result, 0);

    bool isRemoved = mWindowManager->removeWindow(mWindow);
    EXPECT_TRUE(isRemoved);

    status = mWindowManager->getService()->removeWindowToken(mToken, 1);
    EXPECT_TRUE(status.isOk());
}

TEST_F(IWindowManagerTest, Relayout) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, LayoutParams::TYPE_APPLICATION, 1,
                                                          "com.vela.test.relayout");
    EXPECT_TRUE(status.isOk());

    int32_t result = mWindowManager->attachIWindow(mWindow);
    EXPECT_EQ(result, 0);

    status =
            mWindowManager->getService()->updateWindowTokenVisibility(mToken,
                                                                      LayoutParams::WINDOW_VISIBLE);
    EXPECT_TRUE(status.isOk());

    mWindowManager->relayoutWindow(mWindow);

    status = mWindowManager->getService()->updateWindowTokenVisibility(mToken,
                                                                       LayoutParams::WINDOW_GONE);
    EXPECT_TRUE(status.isOk());

    bool isRemoved = mWindowManager->removeWindow(mWindow);
    EXPECT_TRUE(isRemoved);

    status = mWindowManager->getService()->removeWindowToken(mToken, 1);
    EXPECT_TRUE(status.isOk());
}

TEST_F(IWindowManagerTest, UpdateWindowTokenVisibility) {
    Status status = Status::ok();
    status = mWindowManager->getService()->addWindowToken(mToken, LayoutParams::TYPE_APPLICATION, 1,
                                                          "com.vela.test.updatewindowtoken");
    EXPECT_TRUE(status.isOk());

    status = mWindowManager->getService()->updateWindowTokenVisibility(mToken,
                                                                       LayoutParams::WINDOW_HOLD);
    EXPECT_TRUE(status.isOk());

    status =
            mWindowManager->getService()->updateWindowTokenVisibility(mToken,
                                                                      LayoutParams::WINDOW_VISIBLE);
    EXPECT_TRUE(status.isOk());

    status = mWindowManager->getService()->removeWindowToken(mToken, 1);
    EXPECT_TRUE(status.isOk());
}

extern "C" int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
} // namespace wm
} // namespace os
