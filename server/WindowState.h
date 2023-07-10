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

#include "WindowManagerService.h"
#include "WindowNode.h"
#include "WindowToken.h"
#include "os/wm/IWindowManager.h"
#include "wm/LayoutParams.h"
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
    WindowState(WindowManagerService* service, const sp<IWindow>& window, WindowToken* token,
                const LayoutParams& params, int32_t visibility);

    bool isVisible();
    void sendAppVisibilityToClients();
    void setViewVisibility(bool visibility);
    void removeIfPossible();

    std::shared_ptr<InputChannel> createInputChannel(const std::string name);
    std::shared_ptr<SurfaceControl> createSurfaceControl(vector<BufferId> ids);
    std::shared_ptr<BufferConsumer> getBufferConsumer();

    void applyTransaction(LayerState layerState);
    bool scheduleVsync(VsyncRequest vsyncReq);
    bool onVsync();

    std::shared_ptr<WindowToken> getToken() {
        return mToken;
    }

    void setHasSurface(bool hasSurface) {
        mHasSurface = hasSurface;
    }

    BufferItem* acquireBuffer();
    bool releaseBuffer(BufferItem* buffer);

    DISALLOW_COPY_AND_ASSIGN(WindowState);

private:
    sp<IWindow> mClient;
    std::shared_ptr<WindowToken> mToken;
    WindowManagerService* mService;
    std::shared_ptr<SurfaceControl> mSurfaceControl;
    std::shared_ptr<InputChannel> mInputChannel;
    LayoutParams mAttrs;
    VsyncRequest mVsyncRequest;
    int mFrameReq;
    bool mVisibility;
    bool mHasSurface;

    WindowNode* mNode;
};

} // namespace wm
} // namespace os
