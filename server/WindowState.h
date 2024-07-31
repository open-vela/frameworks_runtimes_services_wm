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

class WindowState {
public:
    WindowState();
    ~WindowState();
    WindowState(WindowManagerService* service, const sp<IWindow>& window,
                shared_ptr<WindowToken> token, const LayoutParams& params, int32_t visibility,
                bool enableInput);

    bool isVisible();
    void sendAppVisibilityToClients(int32_t visibility);
    void setVisibility(int32_t visibility);
    void removeIfPossible();
    void removeImmediately();

    std::shared_ptr<InputDispatcher> createInputDispatcher(const std::string& name);
    std::shared_ptr<SurfaceControl> createSurfaceControl(const std::vector<BufferId>& ids);
    std::shared_ptr<BufferConsumer> getBufferConsumer();
    void destroySurfaceControl();

    void applyTransaction(LayerState layerState);
    bool scheduleVsync(VsyncRequest vsyncReq);
    VsyncRequest onVsync();
    bool sendInputMessage(const InputMessage* ie);

    std::shared_ptr<WindowToken> getToken() {
        return mToken;
    }

    sp<IWindow>& getClient() {
        return mClient;
    }

    void setHasSurface(bool hasSurface) {
        mHasSurface = hasSurface;
    }

    BufferItem* acquireBuffer();
    bool releaseBuffer(BufferItem* buffer);

    void setLayoutParams(LayoutParams attrs);
    uint32_t getSurfaceSize();

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    void onAnimationFinished(WindowAnimStatus status);
#endif

    DISALLOW_COPY_AND_ASSIGN(WindowState);

private:
    sp<IWindow> mClient;
    std::shared_ptr<WindowToken> mToken;
    WindowManagerService* mService;
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputDispatcher> mInputDispatcher;
    LayoutParams mAttrs;
    VsyncRequest mVsyncRequest;
    uint32_t mFrameReq;
    int32_t mVisibility;
    bool mHasSurface;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    bool mFrameWaiting;
    bool mAnimRunning;
    WindowAnimator* mWinAnimator;
#endif
    WindowNode* mNode;
    enum {
        WS_ALLOW_REMOVING = 1 << 0,
        WS_REMOVED = 1 << 1,
    };
    int32_t mFlags;
    bool mNeedInput;
};

} // namespace wm
} // namespace os
