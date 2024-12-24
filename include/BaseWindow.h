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
#include "wm/InputMessage.h"
#include "wm/InputMonitor.h"
#include "wm/LayoutParams.h"
#include "wm/WindowEventListener.h"
#include "wm/WindowFrames.h"

namespace os {
namespace wm {

/**
 * @brief MockUI event types for the DummyDriver.
 */
enum { MOCKUI_EVENT_DRAW = 1, MOCKUI_EVENT_CLICK = 2, MOCKUI_EVENT_POSTDRAW = 3 };

/**
 * @brief Callback type for MockUI events.
 *
 * @param data A pointer to the event data.
 * @param size Size of the event data.
 * @param event The event type.
 */
using MOCKUI_EVENT_CALLBACK = std::function<void(void*, uint32_t, uint32_t)>;

class BufferProducer;
class UIDriverProxy;
class WindowManager;
class InputChannel;
class SurfaceControl;

using android::sp;
using android::binder::Status;

/**
 * @class BaseWindow
 * @brief Represents a window in the window manager.
 *
 * This class is responsible for handling window-related operations such as
 * movement, resizing, visibility, and event handling.
 */
class BaseWindow : public std::enable_shared_from_this<BaseWindow> {
public:
    /**
     * @brief Inner class representing a Binder interface for BaseWindow.
     */
    class W : public BnWindow {
    public:
        W(BaseWindow* win) : mBaseWindow(win) {}
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
        BaseWindow* mBaseWindow;
    };

    BaseWindow(::os::app::Context* context, WindowManager* wm);

    ~BaseWindow();

    /**
     * @brief Schedules a Vsync event.
     *
     * @param freq The Vsync request frequency.
     * @return True if the scheduling is successful, false otherwise.
     */
    bool scheduleVsync(VsyncRequest freq);

    /**
     * @brief Retrieves the IWindow interface pointer.
     *
     * @return Shared pointer to the IWindow interface.
     */
    sp<IWindow> getIWindow() {
        return mIWindow;
    }

    /**
     * @brief Retrieves the root object.
     *
     * @return Pointer to the root object.
     */
    void* getRoot();

    /**
     * @brief Retrieves the native display object.
     *
     * @return Pointer to the native display object.
     */
    void* getNativeDisplay();

    /**
     * @brief Sets the UI proxy.
     *
     * @param proxy Shared pointer to the UIDriverProxy.
     */
    void setUIProxy(const std::shared_ptr<UIDriverProxy>& proxy);

    /**
     * @brief Retrieves the UI proxy.
     *
     * @return Reference to the shared pointer of UIDriverProxy.
     */
    std::shared_ptr<UIDriverProxy>& getUIProxy() {
        return mUIProxy;
    }

    /**
     * @brief Sets the type of the window.
     *
     * @param type The type of the window.
     */
    void setType(int32_t type);

    /**
     * @brief Sets the visibility state of the window.
     *
     * @param visible True to make the window visible, false to hide it.
     */
    void setVisible(bool visible);

    /**
     * @brief Sets the layout parameters for the window.
     *
     * @param lp The layout parameters for the window.
     */
    void setLayoutParams(LayoutParams lp);

    /**
     * @brief Retrieves the layout parameters of the window.
     *
     * @return The layout parameters.
     */
    LayoutParams getLayoutParams() {
        return mAttrs;
    }

    /**
     * @brief Retrieves the associated WindowManager.
     *
     * @return Pointer to the WindowManager.
     */
    const WindowManager* getWindowManager() {
        return mWindowManager;
    }

    /**
     * @brief Retrieves the visibility state of the window.
     *
     * @return The visibility state.
     */
    int32_t getVisibility();

    /**
     * @brief Sets the input channel for the window.
     *
     * @param inputChannel Pointer to the InputChannel.
     */
    void setInputChannel(InputChannel* inputChannel);

    /**
     * @brief Sets the surface control for the window.
     *
     * @param surfaceControl Pointer to the SurfaceControl.
     */
    void setSurfaceControl(SurfaceControl* surfaceControl);

    /**
     * @brief Reads an input event.
     *
     * @param message Pointer to the InputMessage to be read.
     * @return True if the event is successfully read, false otherwise.
     */
    bool readEvent(InputMessage* message);

    /**
     * @brief Retrieves the application context.
     *
     * @return Pointer to the application context.
     */
    ::os::app::Context* getContext() {
        return mContext;
    }

    /**
     * @brief Handles the destruction of the window.
     */
    void doDie();

    /**
     * @brief Sets an event listener for window events.
     *
     * @param listener Pointer to the WindowEventListener.
     */
    void setEventListener(WindowEventListener* listener);

    DISALLOW_COPY_AND_ASSIGN(BaseWindow);

    /**
     * @brief Traces the frame if enabled.
     *
     * @param enable True to enable frame tracing, false to disable.
     */
    void traceFrame(bool enable);

private:
    void onFrame(int32_t seq);
    void bufferReleased(int32_t bufKey);

    std::shared_ptr<BufferProducer> getBufferProducer();
    void updateOrCreateBufferQueue();
    void handleOnFrame(int32_t seq);
    void clearSurfaceBuffer();

    ::os::app::Context* mContext;
    WindowManager* mWindowManager;

    LayoutParams mAttrs;
    sp<W> mIWindow;
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputMonitor> mInputMonitor;
    std::shared_ptr<UIDriverProxy> mUIProxy;
    VsyncRequest mVsyncRequest;
    bool mAppVisible;
    atomic_bool mFrameDone;
    bool mSurfaceBufferReady;
    bool mTraceFrame;
    void* mFrameTimeInfo;
};

} // namespace wm
} // namespace os
