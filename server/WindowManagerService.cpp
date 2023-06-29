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

#define LOG_TAG "WindowManagerService"

#include "WindowManagerService.h"

#include <binder/IPCThreadState.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/Log.h>

#include "RootContainer.h"
#include "WindowState.h"
#include "WindowToken.h"
#include "wm/SurfaceControl.h"

static const std::string graphicsPath = "/data/graphics/";

namespace os {
namespace wm {

static inline std::string getUniqueId() {
    return std::to_string(std::rand());
}

static inline BufferId createBufferId(int32_t pid) {
    std::string bufferPath = graphicsPath + std::to_string(pid) + "/bq/" + getUniqueId();
    int32_t fd = shm_open(bufferPath.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        ALOGE("Failed to create shared memory,%s", strerror(errno));
    }
    std::hash<std::string> hasher;
    int32_t bufferKey = hasher(bufferPath);
    return {bufferKey, fd};
}

enum {
    MSG_DO_FRAME = 1001,
};

// TODO: should config it
static const int frameInNs = 16 * 1000000;

class UIFrameHandler : public MessageHandler {
public:
    UIFrameHandler(RootContainer* container) {
        mContainer = container;
    }

    virtual void handleMessage(const Message& message) {
        if (message.what == MSG_DO_FRAME) {
            Looper::getForThread()->sendMessageDelayed(frameInNs, this, Message(MSG_DO_FRAME));
            mContainer->drawFrame();
        }
    }

private:
    RootContainer* mContainer;
};

WindowManagerService::WindowManagerService() {
    mLooper = Looper::getForThread();

    mContainer = new RootContainer();
    if (mLooper) {
        if (mContainer->getSyncMode() == DSM_TIMER) {
            mFrameHandler = new UIFrameHandler(mContainer);
            mLooper->sendMessageDelayed(16 * 1000000, mFrameHandler, Message(MSG_DO_FRAME));
        } else {
            int fd, events;
            if (mContainer->getFdInfo(&fd, &events) == 0) {
                mLooper->addFd(fd, android::Looper::POLL_CALLBACK, events,
                               RootContainer::handleEvent, (void*)mContainer);
            }
        }
    }
}

WindowManagerService::~WindowManagerService() {
    mFrameHandler.clear();
    delete mContainer;
}

// implement AIDL interfaces
Status WindowManagerService::getPhysicalDisplayInfo(int32_t displayId, DisplayInfo* info,
                                                    int32_t* _aidl_return) {
    // TODO
    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::addWindow(const sp<IWindow>& window, const LayoutParams& attrs,
                                       int32_t visibility, int32_t displayId, int32_t userId,
                                       InputChannel* outInputChannel, int32_t* _aidl_return) {
    sp<IBinder> client = IInterface::asBinder(window);
    WindowStateMap::iterator itState = mWindowMap.find(client);
    if (itState != mWindowMap.end()) {
        ALOGI("window is already existed");
        *_aidl_return = -1; // ERROR
        return Status::ok();
    }

    sp<IBinder> token = attrs.mToken; // get token from attrs
    auto itToken = mTokenMap.find(token);
    if (itToken == mTokenMap.end()) {
        *_aidl_return = -1; // ERROR
        return Status::ok();
    }
    WindowToken* winToken = itToken->second;

    WindowState* win = new WindowState(this, window, winToken, attrs, visibility);
    mWindowMap.emplace(client, win);

    winToken->addWindow(win);

    if (outInputChannel != nullptr) {
        int32_t pid = IPCThreadState::self()->getCallingPid();
        std::string name = graphicsPath + std::to_string(pid) + "/event/" + getUniqueId();
        std::shared_ptr<InputChannel> inputChannel = win->createInputChannel(name);
        outInputChannel->copyFrom(*inputChannel);
    }

    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::removeWindow(const sp<IWindow>& window) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::relayout(const sp<IWindow>& window, const LayoutParams& attrs,
                                      int32_t requestedWidth, int32_t requestedHeight,
                                      int32_t visibility, SurfaceControl* outSurfaceControl,
                                      int32_t* _aidl_return) {
    *_aidl_return = -1;
    sp<IBinder> client = IInterface::asBinder(window);
    WindowState* win;
    auto it = mWindowMap.find(client);
    if (it != mWindowMap.end()) {
        ALOGI("find winstate in map");
        win = it->second;
    } else {
        win = nullptr;
        return Status::ok();
    }

    if (visibility) {
        ALOGI("createSurfaceControl() called");
        *_aidl_return = createSurfaceControl(outSurfaceControl, win);
    } else {
        // TODO:destorySurfaceControl
    }

    return Status::ok();
}

Status WindowManagerService::isWindowToken(const sp<IBinder>& binder, bool* _aidl_return) {
    // TODO
    *_aidl_return = 0;
    return Status::ok();
}

Status WindowManagerService::addWindowToken(const sp<IBinder>& token, int32_t type,
                                            int32_t displayId) {
    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        ALOGW("windowToken is already existed");
        return Status::ok();
    } else {
        WindowToken* windToken = new WindowToken(this, token, type, displayId);
        mTokenMap.emplace(token, windToken);
    }
    return Status::ok();
}

Status WindowManagerService::removeWindowToken(const sp<IBinder>& token, int32_t displayId) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::updateWindowTokenVisibility(const sp<IBinder>& token,
                                                         int32_t visibility) {
    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        it->second->setClientVisible(visibility);
    }
    return Status::ok();
}

Status WindowManagerService::applyTransaction(const vector<LayerState>& state) {
    // TODO
    return Status::ok();
}

Status WindowManagerService::requestVsync(const sp<IWindow>& window, VsyncRequest freq) {
    // TODO
    return Status::ok();
}

int32_t WindowManagerService::createSurfaceControl(SurfaceControl* outSurfaceControl,
                                                   WindowState* win) {
    int32_t result = 0;

    // double buffer: Create shared memory
    int32_t pid = IPCThreadState::self()->getCallingPid();

    vector<BufferId> ids;
    for (int i = 0; i < 2; i++) {
        ids.push_back(createBufferId(pid));
    }
    std::shared_ptr<SurfaceControl> surfaceControl = win->createSurfaceControl(ids);

    if (surfaceControl != nullptr) {
        outSurfaceControl->copyFrom(*surfaceControl);
    }

    return result;
}

} // namespace wm
} // namespace os
