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

#include <uv.h>

#include <map>
#include <vector>

#include "DeviceEventListener.h"
#include "GestureDetector.h"
#include "WindowConfig.h"
#include "app/UvLoop.h"
#include "os/wm/BnWindowManager.h"

/**
 * @namespace os::wm
 * @brief The namespace for window management and related functionalities.
 */
namespace os {
namespace wm {

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
class WindowAnimEngine;
#endif
class RootContainer;
class WindowState;
class WindowToken;
class InputDispatcher;

typedef std::map<sp<IBinder>, std::shared_ptr<WindowToken>> WindowTokenMap;
typedef std::map<sp<IBinder>, WindowState*> WindowStateMap;
typedef std::map<sp<IBinder>, std::shared_ptr<InputDispatcher>> InputMonitorMap;

/**
 * @class WindowManagerService
 * @brief This class provides services for window management.
 *
 * The WindowManagerService is responsible for managing window states,
 * handling input events, and coordinating between different windows and
 * their interactions. It serves as the primary interface for window
 * operations in the system.
 */
class WindowManagerService : public BnWindowManager, public DeviceEventListener {
public:
    WindowManagerService(std::shared_ptr<::os::app::UvLoop> uvLooper);

    ~WindowManagerService();

    static inline const char* name() {
        return "window";
    }

    /**
     * @brief Gets physical display information.
     *
     * This method retrieves the information about the physical display
     * associated with the specified display ID.
     *
     * @param displayId ID of the display to retrieve information for.
     * @param info Pointer to a DisplayInfo structure to store the retrieved information.
     * @param _aidl_return Pointer used to return the operation status.
     * @return Status indicating the success or failure of the operation.
     */
    Status getPhysicalDisplayInfo(int32_t displayId, DisplayInfo* info, int32_t* _aidl_return);

    /**
     * @brief Adds a new window.
     *
     * This method registers a new window with the window manager and
     * associates it with the specified attributes.
     *
     * @param window A pointer to the window interface.
     * @param attrs The layout parameters for the window.
     * @param visibility The visibility state of the window.
     * @param displayId ID of the display where the window will be shown.
     * @param userId ID of the user to which the window belongs.
     * @param outInputChannel Pointer for returning an InputChannel associated with the window.
     * @param _aidl_return Pointer used to return the operation status.
     * @return Status indicating the success or failure of the operation.
     */
    Status addWindow(const sp<IWindow>& window, const LayoutParams& attrs, int32_t visibility,
                     int32_t displayId, int32_t userId, InputChannel* outInputChannel,
                     int32_t* _aidl_return);

    /**
     * @brief Removes a window.
     *
     * This method unregisters a window from the system and releases any
     * associated resources.
     *
     * @param window A pointer to the window interface to be removed.
     * @return Status indicating the success or failure of the operation.
     */
    Status removeWindow(const sp<IWindow>& window);

    /**
     * @brief Relayouts an existing window.
     *
     * This method adjusts the properties of a window without removing it,
     * allowing changes to its layout parameters.
     *
     * @param window A pointer to the window to be relaid out.
     * @param attrs The new layout parameters for the window.
     * @param requestedWidth The requested width for the window.
     * @param requestedHeight The requested height for the window.
     * @param visibility The visibility state of the window.
     * @param outSurfaceControl Pointer for returning the SurfaceControl associated with the window.
     * @param _aidl_return Pointer used to return the operation status.
     * @return Status indicating the success or failure of the operation.
     */
    Status relayout(const sp<IWindow>& window, const LayoutParams& attrs, int32_t requestedWidth,
                    int32_t requestedHeight, int32_t visibility, SurfaceControl* outSurfaceControl,
                    int32_t* _aidl_return);

    /**
     * @brief Checks if the provided IBinder is a valid window token.
     *
     * @param binder The IBinder to check.
     * @param _aidl_return Pointer used to return the result of the check.
     * @return Status indicating the success or failure of the operation.
     */
    Status isWindowToken(const sp<IBinder>& binder, bool* _aidl_return);

    /**
     * @brief Adds a window token to the system.
     *
     * This method associates a token with a specified type and displays it on the given display ID.
     *
     * @param token The IBinder token to be added.
     * @param type The type of the window associated with the token.
     * @param displayId The ID of the display.
     * @param packageName The package name of the application to which the window belongs.
     * @return Status indicating the success or failure of the operation.
     */
    Status addWindowToken(const sp<IBinder>& token, int32_t type, int32_t displayId,
                          const std::string& packageName);

    /**
     * @brief Removes a window token from the system.
     *
     * @param token The IBinder token to be removed.
     * @param displayId The ID of the display associated with the token.
     * @return Status indicating the success or failure of the operation.
     */
    Status removeWindowToken(const sp<IBinder>& token, int32_t displayId);

    /**
     * @brief Updates the visibility state of a window token.
     *
     * @param token The IBinder token to update.
     * @param visibility The new visibility state for the token.
     * @return Status indicating the success or failure of the operation.
     */
    Status updateWindowTokenVisibility(const sp<IBinder>& token, int32_t visibility);

    /**
     * @brief Applies the pending transactions for layers.
     *
     * This method processes and applies the transaction updates for window layers.
     *
     * @param state A vector of LayerState containing the updates to apply.
     * @return Status indicating the success or failure of the operation.
     */
    Status applyTransaction(const vector<LayerState>& state);

    /**
     * @brief Requests a Vsync signal for a specific window.
     *
     * @param window A pointer to the window for which Vsync is requested.
     * @param freq The requested Vsync frequency.
     * @return Status indicating the success or failure of the operation.
     */
    Status requestVsync(const sp<IWindow>& window, VsyncRequest freq);

    /**
     * @brief Responds to Vsync events.
     *
     * This method handles the logic for Vsync notifications.
     *
     * @return True if the event was handled successfully, false otherwise.
     */
    bool responseVsync() override;

    /**
     * @brief Monitors input events for a specific token.
     *
     * This method establishes an InputChannel for the given token.
     *
     * @param token The IBinder token for which to monitor input.
     * @param name The name for the input monitoring session.
     * @param displayId ID of the display associated with the token.
     * @param outInputChannel Pointer for returning the InputChannel.
     * @return Status indicating the success or failure of the operation.
     */
    Status monitorInput(const sp<IBinder>& token, const ::std::string& name, int32_t displayId,
                        InputChannel* outInputChannel);

    /**
     * @brief Releases input monitoring for a specified token.
     *
     * @param token The IBinder token to release.
     * @return Status indicating the success or failure of the operation.
     */
    Status releaseInput(const sp<IBinder>& token);

    /**
     * @brief Retrieves information about a window.
     *
     * @param window The window for which to retrieve information.
     * @param outWindowInfo Pointer for returning the WindowInfo.
     * @return Status indicating the success or failure of the operation.
     */
    Status getWindowInfo(const sp<IWindow>& window, WindowInfo* outWindowInfo);

    /**
     * @brief Responds to input messages.
     *
     * This method handles the reception and processing of input messages.
     *
     * @param msg Pointer to the InputMessage to process.
     * @return True if the message was processed successfully, false otherwise.
     */
    bool responseInput(InputMessage* msg) override;

    /**
     * @brief Retrieves the root container associated with the WindowManagerService.
     *
     * @return Pointer to the RootContainer.
     */
    RootContainer* getRootContainer() {
        return mContainer;
    }

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    /**
     * @brief Retrieves the animation engine used for window transitions.
     *
     * @return Handle to the animation engine.
     */
    AnimEngineHandle getAnimEngine();

    /**
     * @brief Retrieves the animation configuration for a specific window state.
     *
     * @param animMode The mode of the animation.
     * @param win Pointer to the WindowState for which to get the animation config.
     * @return The animation configuration as a string.
     */
    std::string getAnimConfig(bool animMode, WindowState* win);
#endif

    /**
     * @brief Cleans up resources after a window is removed.
     *
     * @param state Pointer to the WindowState to clean up.
     */
    void postWindowRemoveCleanup(WindowState* state);

    /**
     * @brief Removes a window token from internal management.
     *
     * @param token Reference to the IBinder token to remove.
     * @return True if the token is successfully removed, false otherwise.
     */
    bool removeWindowTokenInner(sp<IBinder>& token);

    /**
     * @brief Checks if the WindowManagerService is ready for operation.
     *
     * @return True if the service is ready, false otherwise.
     */
    bool ready();

private:
    class WindowDeathRecipient : public IBinder::DeathRecipient {
    public:
        WindowDeathRecipient(WindowManagerService* service) : mService(service) {}

        /**
         * @brief Handles the event when the binder dies.
         *
         * This method is invoked when the associated IBinder goes out of scope.
         *
         * @param who Weak pointer to the IBinder that died.
         */
        virtual void binderDied(const wp<IBinder>& who);

    private:
        WindowManagerService* mService;
    };

    WindowTokenMap mTokenMap;
    WindowStateMap mWindowMap;
    RootContainer* mContainer;
    std::shared_ptr<::os::app::UvLoop> mUvLooper;
    InputMonitorMap mInputMonitorMap;
    sp<WindowDeathRecipient> mWindowDeathRecipient;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    WindowAnimEngine* mWinAnimEngine;
#endif
    GestureDetector mGestureDetector;
};

} // namespace wm
} // namespace os
