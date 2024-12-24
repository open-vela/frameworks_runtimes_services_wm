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

#include <vector>

#include "pm/PackageInfo.h"

/**
 * @namespace os::wm
 * @brief The namespace for window management related classes and functionalities.
 */
namespace os {
namespace wm {

using android::IBinder;
using android::sp;

class WindowManagerService;
class WindowState;

/**
 * @class WindowToken
 * @brief Represents a token for a window in the window manager.
 *
 * This class holds information about a group of windows identified
 * by a unique token. It manages the visibility and state of those
 * windows within a particular display.
 */
class WindowToken {
public:
    WindowToken(WindowManagerService* service, const sp<IBinder>& token, int32_t type,
                int32_t displayId, int32_t clientPid, const std::string& packageName);

    ~WindowToken();

    /**
     * @brief Adds a window to this token.
     *
     * This method associates the specified WindowState with this token.
     *
     * @param win Pointer to the WindowState to add.
     */
    void addWindow(WindowState* win);

    /**
     * @brief Removes a window from this token.
     *
     * This method disassociates the specified WindowState from this token.
     *
     * @param win Pointer to the WindowState to remove.
     */
    void removeWindow(WindowState* win);

    /**
     * @brief Retrieves the visibility state of the associated client.
     *
     * @return The client's visibility state.
     */
    int32_t getClientVisibility() {
        return mClientVisibility;
    }

    /**
     * @brief Sets the visibility state of the associated client.
     *
     * @param visibility The new visibility state for the client.
     */
    void setClientVisibility(int32_t visibility);

    /**
     * @brief Retrieves the process ID of the associated client.
     *
     * @return The process ID of the client.
     */
    int getClientPid() {
        return mClientPid;
    }

    /**
     * @brief Checks if this token has no associated windows.
     *
     * @return True if there are no associated windows, false otherwise.
     */
    bool isEmpty() {
        return mChildren.empty();
    }

    /**
     * @brief Sets whether to persist the token when empty.
     *
     * @param persistOnEmpty Flag indicating whether to persist on empty.
     */
    void setPersistOnEmpty(bool persistOnEmpty) {
        mPersistOnEmpty = persistOnEmpty;
    }

    /**
     * @brief Checks if the token should persist when empty.
     *
     * @return True if the token should persist on being empty, otherwise false.
     */
    bool isPersistOnEmpty() {
        return mPersistOnEmpty;
    }

    /**
     * @brief Attempts to remove the token if possible.
     *
     * This method disassociates the token from its windows and cleans up.
     */
    void removeIfPossible();

    /**
     * @brief Retrieves the type of the windows associated with this token.
     *
     * @return The type of the windows.
     */
    int32_t getType() {
        return mType;
    }

    const std::string& getWindowEnterAnimType() {
        return mPackageInfo.windowEnterAnim;
    }

    const std::string& getWindowExitAnimType() {
        return mPackageInfo.windowExitAnim;
    }

private:
    WindowManagerService* mService;
    sp<IBinder> mToken;
    int32_t mType;
    std::vector<WindowState*> mChildren;
    int32_t mClientVisibility;
    int mClientPid;
    bool mPersistOnEmpty;
    bool mRemoved;
    ::os::pm::PackageInfo mPackageInfo;
};

} // namespace wm
} // namespace os
