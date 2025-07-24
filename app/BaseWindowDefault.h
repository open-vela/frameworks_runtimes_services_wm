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

#include <android-base/macros.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include <atomic>

#include "BaseWindow.h"
#include "WindowManagerDefault.h"
#include "os/wm/BnWindow.h"
#include "os/wm/VsyncRequest.h"

namespace os {
namespace wm {

class BufferProducer;
class UIDriverProxy;
class WindowManagerDefault;
class InputChannel;
class SurfaceControl;

using android::sp;
using android::binder::Status;

/**
 * @class BaseWindowDefault
 * @brief Represents a window in the window manager.
 *
 * This class is responsible for handling window-related operations such as
 * movement, resizing, visibility, and event handling.
 */
class BaseWindowDefault : public BaseWindow {
public:
    /**
     * @brief Inner class representing a Binder interface for BaseWindowDefault.
     */
    class W : public BnWindow {
    public:
        W(BaseWindowDefault* win) : mBaseWindow(win) {}
        ~W() {}

        /**
         * @brief Notifies that the window has moved to a new position.
         *
         * @param newX New X coordinate of the window.
         * @param newY New Y coordinate of the window.
         * @return Status of the operation.
         */
        Status moved(int32_t newX, int32_t newY) override;

        /**
         * @brief Notifies that the window has been resized.
         *
         * @param frames New layout frames of the window.
         * @param displayId ID of the display.
         * @return Status of the operation.
         */
        Status resized(const WindowFrames& frames, int32_t displayId) override;

        /**
         * @brief Notifies the visibility state of the application.
         *
         * @param visible True if the application is visible, false otherwise.
         * @return Status of the operation.
         */
        Status dispatchAppVisibility(bool visible) override;

        /**
         * @brief Notifies that a new frame has been rendered.
         *
         * @param seq Sequence number of the frame.
         * @return Status of the operation.
         */
        Status onFrame(int32_t seq) override;

        /**
         * @brief Notifies that a buffer has been released.
         *
         * @param bufKey Key of the released buffer.
         * @return Status of the operation.
         */
        Status bufferReleased(int32_t bufKey) override;
        /**
         * @brief Clears the internal state.
         */
        void clear();

    private:
        BaseWindowDefault* mBaseWindow;
    };

    BaseWindowDefault(::os::app::Context* context, WindowManagerDefault* wm);
    ~BaseWindowDefault();

    DISALLOW_COPY_AND_ASSIGN(BaseWindowDefault);

    bool onCreateStage() override;

    bool onDestroyStage() override;

    void* getRoot() override;

    void setType(int32_t type) override;

    void setVisible(bool visible) override;

    void setLayoutParams(LayoutParams lp) override;

    LayoutParams getLayoutParams() override {
        return mAttrs;
    }

    const WindowManager* getWindowManager() override {
        return mWindowManager;
    }

    int32_t getVisibility() override;

    ::os::app::Context* getContext() override {
        return mContext;
    }

    void* getNativeDisplay() override;

    void setEventListener(WindowEventListener* listener) override;

    void traceFrame(bool enable) override;

    sp<IWindow> getIWindow() {
        return mIWindow;
    }

    void doDie();

    void setInputChannel(InputChannel* inputChannel);

    void setSurfaceControl(SurfaceControl* surfaceControl);

    /**
     * @brief Schedules a Vsync event.
     *
     * @param freq The Vsync request frequency.
     * @return True if the scheduling is successful, false otherwise.
     */
    bool scheduleVsync(VsyncRequest freq);

    void initRoot(void* root);

    void initUIProxy(const std::shared_ptr<UIDriverProxy>& proxy);

private:
    void onFrame(int32_t seq);
    void bufferReleased(int32_t bufKey);

    std::shared_ptr<BufferProducer> getBufferProducer();
    void updateOrCreateBufferQueue();
    void handleOnFrame(int32_t seq);
    void clearSurfaceBuffer();

    ::os::app::Context* mContext;
    WindowManagerDefault* mWindowManager;

    LayoutParams mAttrs;
    sp<W> mIWindow;
    bool mAppVisible;
    void* mRoot;

    /* for multi-instance mode */
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputMonitor> mInputMonitor;
    std::shared_ptr<UIDriverProxy> mUIProxy;
    VsyncRequest mVsyncRequest;
    atomic_bool mFrameDone;
    bool mSurfaceBufferReady;
    bool mTraceFrame;
    void* mFrameTimeInfo;
};

} // namespace wm
} // namespace os
