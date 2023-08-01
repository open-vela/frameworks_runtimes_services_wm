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

#pragma once

#include <mutex>

#include "BaseWindow.h"
#include "app/Context.h"
#include "os/wm/BnWindowManager.h"

namespace os {
namespace wm {

using android::sp;

class BaseWindow;
class SurfaceTransaction;
class WindowManager {
public:
    ~WindowManager();

    static inline const char* name() {
        return "window";
    }
    static WindowManager* getInstance();

    std::shared_ptr<BaseWindow> newWindow(::os::app::Context* context);
    int32_t attachIWindow(std::shared_ptr<BaseWindow> window);
    void relayoutWindow(std::shared_ptr<BaseWindow> window);
    bool removeWindow(std::shared_ptr<BaseWindow> window);
    sp<IWindowManager>& getService();

    std::shared_ptr<SurfaceTransaction>& getTransaction() {
        return mTransaction;
    }

private:
    WindowManager();

    std::mutex mLock;
    vector<std::shared_ptr<BaseWindow>> mWindows;
    sp<IWindowManager> mService;
    std::shared_ptr<SurfaceTransaction> mTransaction;
};

} // namespace wm
} // namespace os
