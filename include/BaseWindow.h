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

#include <android-base/macros.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include <atomic>

#include "WindowManager.h"
#include "app/Context.h"
#include "app/UvLoop.h"
#include "os/wm/BnWindow.h"
#include "os/wm/VsyncRequest.h"
#include "wm/LayoutParams.h"
#include "wm/WindowFrames.h"

namespace os {
namespace wm {

// for MockUI (DummyDriver)
enum {
    MOCKUI_EVENT_DRAW = 1,
    MOCKUI_EVENT_CLICK = 2,
};
// data, size, event
using MOCKUI_EVENT_CALLBACK = std::function<void(void*, uint32_t, uint32_t)>;
// end for MockUI (DummyDriver)

class BufferProducer;
class UIDriverProxy;
class WindowManager;
class InputChannel;
class SurfaceControl;

using android::sp;
using android::binder::Status;

class BaseWindow : public std::enable_shared_from_this<BaseWindow> {
public:
    class W : public BnWindow {
    public:
        W(BaseWindow* win) : mBaseWindow(win) {}
        ~W() {}

        Status moved(int32_t newX, int32_t newY) override;
        Status resized(const WindowFrames& frames, int32_t displayId) override;
        Status dispatchAppVisibility(bool visible) override;
        Status onFrame(int32_t seq) override;
        Status bufferReleased(int32_t bufKey) override;

    private:
        BaseWindow* mBaseWindow;
    };

    BaseWindow(::os::app::Context* context);
    ~BaseWindow();

    bool scheduleVsync(VsyncRequest freq);

    sp<IWindow> getIWindow() {
        return mIWindow;
    }

    void* getRoot();

    void setUIProxy(const std::shared_ptr<UIDriverProxy>& proxy) {
        mUIProxy = proxy;
    }

    std::shared_ptr<UIDriverProxy>& getUIProxy() {
        return mUIProxy;
    }

    void setLayoutParams(LayoutParams lp) {
        mAttrs = lp;
    }

    LayoutParams getLayoutParams() {
        return mAttrs;
    }

    void setWindowManager(WindowManager* wm);

    const WindowManager* getWindowManager() {
        return mWindowManager;
    }

    std::shared_ptr<BufferProducer> getBufferProducer();

    void doDie();
    bool getAppVisible() {
        return mAppVisible;
    }
    void dispatchAppVisibility(bool visible);
    void handleAppVisibility(bool visible);

    void onFrame(int32_t seq);
    void handleOnFrame(int32_t seq);

    void bufferReleased(int32_t bufKey);
    void handleBufferReleased(int32_t bufKey);

    void setInputChannel(InputChannel* inputChannel);
    void setSurfaceControl(SurfaceControl* surfaceControl);

    void setMockUIEventCallback(const MOCKUI_EVENT_CALLBACK& cb);

    DISALLOW_COPY_AND_ASSIGN(BaseWindow);

private:
    void updateOrCreateBufferQueue();

    ::os::app::Context* mContext;
    WindowManager* mWindowManager;

    LayoutParams mAttrs;
    sp<W> mIWindow;
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputChannel> mInputChannel;
    std::shared_ptr<UIDriverProxy> mUIProxy;
    uv_poll_t* mPoll;
    VsyncRequest mVsyncRequest;
    bool mAppVisible;
    atomic_bool mFrameDone;
};

} // namespace wm
} // namespace os
