/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "BaseWindowDefault.h"
#include "WindowManager.h"

namespace os {
namespace wm {

class BaseWindowDefault;
class SurfaceTransaction;

class WindowManagerDefault : public WindowManager {
public:
    WindowManagerDefault();
    ~WindowManagerDefault();

    std::shared_ptr<BaseWindow> newWindow(::os::app::Context* context) override;
    int32_t attachIWindow(std::shared_ptr<BaseWindow> window) override;
    void relayoutWindow(std::shared_ptr<BaseWindow> window) override;
    bool removeWindow(std::shared_ptr<BaseWindow> window) override;
    bool dumpWindows() override;
    void getDisplayInfo(uint32_t* width, uint32_t* height) const override;
    sp<IWindowManager>& getService() override;

    std::shared_ptr<SurfaceTransaction>& getTransaction();

private:
    std::mutex mLock;
    vector<std::shared_ptr<BaseWindowDefault>> mWindows;
    sp<IWindowManager> mService;
    uint32_t mDispWidth, mDispHeight;

    /* for multi-instance mode */
    std::shared_ptr<SurfaceTransaction> mTransaction;
    uv_timer_t mEventTimer;
    bool mTimerInited;
};

} // namespace wm
} // namespace os
