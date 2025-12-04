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

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include "InputDispatcher.h"
#include "WindowConfig.h"
#include "WindowManagerService.h"
#include "WindowNode.h"
#include "WindowToken.h"
#include "os/wm/IWindowManager.h"
#include "wm/LayoutParams.h"

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
#include "WindowAnimator.h"
#endif

namespace os {
namespace wm {

using android::sp;

class WindowManagerService;
class SurfaceControl;
class WindowNode;
class WindowToken;
class LayoutParams;

/**
 * @class WindowState
 * @brief Represents the state of a window in the window manager.
 *
 * The WindowState class encapsulates all the properties of a window,
 * including its visibility, layout parameters, and connections to
 * other components like input dispatchers and surface controls.
 * It provides methods to manage the lifecycle and appearance of
 * the window.
 */
class WindowState {
public:
    WindowState();
    ~WindowState();

    WindowState(WindowManagerService* service, const sp<IWindow>& window,
                std::shared_ptr<WindowToken> token, const LayoutParams& params, int32_t visibility,
                bool enableInput);

    /**
     * @brief Checks if the window is currently visible.
     *
     * @return True if the window is visible, false otherwise.
     */
    bool isVisible();

    /**
     * @brief Sends the visibility state of the application to the clients.
     *
     * @param visibility The visibility state to send to the clients.
     */
    void sendAppVisibilityToClients(int32_t visibility);

    /**
     * @brief Sets the visibility state of the window.
     *
     * @param visibility The visibility state to set for the window.
     */
    void setVisibility(int32_t visibility);

    /**
     * @brief Removes the window if possible.
     *
     * This method attempts to remove the window from the window manager.
     */
    void removeIfPossible();

    /**
     * @brief Immediately removes the window from the window manager.
     *
     * This method forcefully removes the window, releasing any resources associated with it.
     */
    void removeImmediately();

    /**
     * @brief Retrieves the associated WindowToken.
     *
     * @return A shared pointer to the WindowToken.
     */
    std::shared_ptr<WindowToken> getToken() {
        return mToken;
    }

    /**
     * @brief Retrieves the client associated with this window.
     *
     * @return A reference to the associated IWindow client.
     */
    sp<IWindow>& getClient() {
        return mClient;
    }

    /**
     * @brief Sets the layout parameters for this window.
     *
     * @param attrs The new LayoutParams to apply to the window.
     */
    void setLayoutParams(LayoutParams attrs);

    /**
     * @brief Sets the client exited flag.
     *
     * Marks that the client has exited to avoid sending notifications.
     */
    void setClientExited() {
        mFlags |= WS_CLIENT_EXITED;
    }

    /**
     * @brief Retrieves the size of the surface for this window.
     *
     * @return The size of the current surface in pixels.
     */
    uint32_t getSurfaceSize();

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    /**
     * @brief Called when the animation for this window has finished.
     *
     * @param status The status of the completed animation.
     */
    void onAnimationFinished(WindowAnimStatus status);
#endif

    DISALLOW_COPY_AND_ASSIGN(WindowState);

    /**
     * @brief Sends an input message to this window.
     *
     * @param ie Pointer to the InputMessage to send.
     * @return True if the message was successfully sent, false otherwise.
     */
    bool sendInputMessage(const InputMessage* ie);

    /**
     * @brief Creates a new InputDispatcher for this window.
     *
     * @param name The name to associate with the input dispatcher.
     * @return A shared pointer to the newly created InputDispatcher.
     */
    std::shared_ptr<InputDispatcher> createInputDispatcher(const std::string& name);

    /**
     * @brief Creates a new SurfaceControl for this window.
     *
     * @param ids A vector of BufferId to associate with the surface.
     * @param fmqName The name for the Fast Message Queue.
     * @return A shared pointer to the newly created SurfaceControl.
     */
    std::shared_ptr<SurfaceControl> createSurfaceControl(const std::vector<BufferId>& ids,
                                                         const std::string& fmqName);

    /**
     * @brief Retrieves the BufferConsumer associated with this window.
     *
     * @return A shared pointer to the BufferConsumer for this window.
     */
    std::shared_ptr<BufferConsumer> getBufferConsumer();

    /**
     * @brief Destroys the SurfaceControl associated with this window.
     *
     * This method cleans up the resources associated with the surface control.
     */
    void destroySurfaceControl();

    /**
     * @brief Applies a transaction to the window's layer.
     *
     * @param layerState The state of the layer to apply.
     */
    void applyTransaction(LayerState layerState);

    /**
     * @brief Schedules a Vsync request for this window.
     *
     * @param vsyncReq The VsyncRequest to schedule.
     * @return True if the request was successfully scheduled, false otherwise.
     */
    bool scheduleVsync(VsyncRequest vsyncReq);

    /**
     * @brief Handles the Vsync event for this window.
     *
     * @return The current VsyncRequest state.
     */
    VsyncRequest onVsync();

    /**
     * @brief Sets whether this window has a surface.
     *
     * @param hasSurface True to indicate that the window has a surface, false otherwise.
     */
    void setHasSurface(bool hasSurface) {
        mHasSurface = hasSurface;
    }

    /**
     * @brief Acquires a buffer for this window.
     *
     * @return Pointer to the acquired BufferItem for rendering.
     */
    BufferItem* acquireBuffer();

    /**
     * @brief Releases a buffer previously acquired by this window.
     *
     * @param buffer Pointer to the BufferItem to release.
     * @return True if the release was successful, false otherwise.
     */
    bool releaseBuffer(BufferItem* buffer);

    /**
     * @brief Retrieves the screen associated with this window.
     *
     * @return A pointer to the screen associated with this window.
     */
    void* getClientScreen();
    bool needInput() {
        return mNeedInput;
    }

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    bool windowTransition(bool in);
#endif

private:
    sp<IWindow> mClient;
    std::shared_ptr<WindowToken> mToken;
    WindowManagerService* mService;
    LayoutParams mAttrs;
    uint32_t mFrameReq;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    bool mFrameWaiting;
    bool mAnimRunning;
    WindowAnimator* mWinAnimator;
#endif
    WindowNode* mNode;

    enum {
        WS_ALLOW_REMOVING = 1 << 0,
        WS_REMOVED = 1 << 1,

        /* check refresh timeout, for internal use only */
        WS_CLIENT_TIMEOUT = 1 << 15,
        WS_CLIENT_EXITED = 1 << 16,
    };
    int32_t mFlags;
    bool mNeedInput;

    /* for multi-instance mode */
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputDispatcher> mInputDispatcher;
    bool mHasSurface;
    VsyncRequest mVsyncRequest;
};

} // namespace wm
} // namespace os
