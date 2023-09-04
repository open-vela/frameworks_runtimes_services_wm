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

#include <utils/Looper.h>

#include <map>
#include <vector>

#include "os/wm/BnWindowManager.h"

namespace os {
namespace wm {

class RootContainer;
class WindowState;
class WindowToken;

typedef map<sp<IBinder>, WindowToken*> WindowTokenMap;
typedef map<sp<IBinder>, WindowState*> WindowStateMap;

enum {
    POLL_EVENT_INPUT = Looper::EVENT_INPUT,
    POLL_EVENT_OUTPUT = Looper::EVENT_OUTPUT,
    POLL_EVENT_ERROR = Looper::EVENT_ERROR,
    POLL_EVENT_HANGUP = Looper::EVENT_HANGUP,
    POLL_EVENT_INVALID = Looper::EVENT_INVALID
};

class WindowManagerService : public BnWindowManager {
public:
    WindowManagerService();
    ~WindowManagerService();

    static inline const char* name() {
        return "window";
    }

    // define AIDL interfaces
    Status getPhysicalDisplayInfo(int32_t displayId, DisplayInfo* info, int32_t* _aidl_return);
    Status addWindow(const sp<IWindow>& window, const LayoutParams& attrs, int32_t visibility,
                     int32_t displayId, int32_t userId, InputChannel* outInputChannel,
                     int32_t* _aidl_return);
    Status removeWindow(const sp<IWindow>& window);
    Status relayout(const sp<IWindow>& window, const LayoutParams& attrs, int32_t requestedWidth,
                    int32_t requestedHeight, int32_t visibility, SurfaceControl* outSurfaceControl,
                    int32_t* _aidl_return);

    Status isWindowToken(const sp<IBinder>& binder, bool* _aidl_return);
    Status addWindowToken(const sp<IBinder>& token, int32_t type, int32_t displayId);
    Status removeWindowToken(const sp<IBinder>& token, int32_t displayId);
    Status updateWindowTokenVisibility(const sp<IBinder>& token, int32_t visibility);

    Status applyTransaction(const vector<LayerState>& state);
    Status requestVsync(const sp<IWindow>& window, VsyncRequest freq);
    bool responseVsync();
    void postTimerMessage(int32_t delay);

    // public methods
    RootContainer* getRootContainer() {
        return mContainer;
    }

    bool registerFd(int fd, int events, Looper_callbackFunc cb, void* data);
    void unregisterFd(int fd);

    void doRemoveWindow(const sp<IWindow>& window);

private:
    int32_t createSurfaceControl(SurfaceControl* outSurfaceControl, WindowState* win);

    WindowTokenMap mTokenMap;
    WindowStateMap mWindowMap;

    RootContainer* mContainer;
    sp<Looper> mLooper;

    sp<MessageHandler> mTimerHandler;
    bool mTimerStopped;
};

} // namespace wm
} // namespace os
