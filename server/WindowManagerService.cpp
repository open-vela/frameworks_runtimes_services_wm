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

#define LOG_TAG "WMS"

#include "WindowManagerService.h"

#include <binder/IPCThreadState.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/Log.h>

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
#include "WindowAnimEngine.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#endif
#include "InputDispatcher.h"
#include "RootContainer.h"
#include "WindowState.h"
#include "WindowToken.h"
#include "WindowUtils.h"
#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
static std::map<int, std::string> mAnimConfigMap;
static const std::string animConfigPath = "/data/app/window_anim_config.json";

static const std::string defaultEnterConfigJson =
        R"({
            "fromState": {
                "opacity": 0.0,
                "scale": 0.8
            },
            "toState": {
                "opacity": 1.0,
                "scale": 1.0
            },
            "config": {
                "ease": [
                    "cubicInOut",
                    0.3
                ]
            }
        })";

static const std::string defaultExitConfigJson =
        R"({
            "fromState": {
                "opacity": 1.0,
                "scale": 1.0
            },
            "toState": {
                "opacity": 0.0,
                "scale": 0.8
            },
            "config": {
                "ease": [
                    "cubicInOut",
                    0.2
                ]
            }
        })";

int parseAnimJsonFile(const char* filename) {
    std::ifstream file(filename);
    if (!file) {
        FLOGE("Failed to open file %s", filename);
        return android::NAME_NOT_FOUND;
    }

    std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.IsArray()) {
        for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
            const rapidjson::Value& obj = doc[i];
            if (obj.HasMember("id") && obj.HasMember("action")) {
                int id = obj["id"].GetInt();
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                obj["action"].Accept(writer);
                std::string action = buffer.GetString();
                mAnimConfigMap[id] = action;
                FLOGI("mAnimConfigMap[%d] = %s", id, action.c_str());
            }
        }
    }

    return 0;
}

#endif

static inline std::string getUniqueId() {
    return std::to_string(std::rand());
}

static inline std::string genUniquePath(int32_t pid, const char* prefix,
                                        const char* name = nullptr) {
    std::string path = "/data/graphics/" + std::to_string(pid) + "/" + (prefix ? prefix : "") +
            "/" + getUniqueId();
    if (name) path += "/" + std::string(name);
    FLOGD("%s", path.c_str());
    return path;
}

static inline bool createSharedBuffer(int32_t size, BufferId* id) {
    int32_t pid = IPCThreadState::self()->getCallingPid();

    std::string bufferPath = genUniquePath(pid, "bq");
    int fd = shm_open(bufferPath.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        FLOGE("[%" PRId32 "] Failed to create shared memory, %s", pid, strerror(errno));
        return false;
    }

    if (ftruncate(fd, size) == -1) {
        FLOGE("[%" PRId32 "] Failed to resize shared memory", pid);
        close(fd);
        return false;
    }

    int32_t bufferKey = std::rand();
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    *id = {bufferPath, bufferKey, fd};
#else
    *id = {bufferKey, fd};
#endif

    return true;
}

void WindowManagerService::WindowDeathRecipient::binderDied(const wp<IBinder>& who) {
    FLOGI("IWindow binder Died");
    auto key = who.promote();
    auto it = mService->mWindowMap.find(key);
    if (it != mService->mWindowMap.end()) {
        it->second->removeIfPossible();
        mService->mWindowMap.erase(key);
    }
}

WindowManagerService::WindowManagerService(uv_loop_t* looper) : mLooper(looper) {
    FLOGI("WMS init");
    mContainer = new RootContainer(this, looper);
    mWindowDeathRecipient = sp<WindowDeathRecipient>::make(this);
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    mWinAnimEngine = new WindowAnimEngine();
    int ret = parseAnimJsonFile(animConfigPath.c_str());
    if (ret < 0) {
        ALOGE("read file %s failed,use default anim config", animConfigPath.c_str());
    }
#endif
}

WindowManagerService::~WindowManagerService() {
    mInputMonitorMap.clear();
    delete mContainer;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    delete mWinAnimEngine;
    mAnimConfigMap.clear();
#endif
}

Status WindowManagerService::getPhysicalDisplayInfo(int32_t displayId, DisplayInfo* info,
                                                    int32_t* _aidl_return) {
    WM_PROFILER_BEGIN();
    *_aidl_return = 0;
    if (mContainer) {
        mContainer->getDisplayInfo(info);
        FLOGI("display size (%" PRId32 "x%" PRId32 ")", info->width, info->height);
    }
    WM_PROFILER_END();

    return Status::ok();
}

Status WindowManagerService::addWindow(const sp<IWindow>& window, const LayoutParams& attrs,
                                       int32_t visibility, int32_t displayId, int32_t userId,
                                       InputChannel* outInputChannel, int32_t* _aidl_return) {
    WM_PROFILER_BEGIN();
    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "] visibility(%" PRId32 ") size(%" PRId32 "x%" PRId32 ")", pid, visibility,
          attrs.mWidth, attrs.mHeight);

    if (mWindowMap.size() >= CONFIG_ENABLE_WINDOW_LIMIT_MAX) {
        ALOGE("failure, exceed maximum window limit!");
        *_aidl_return = -1;
        mContainer->showToast("Warn: exceed maximum window limit!", 1500);
        return Status::fromExceptionCode(1, "exceed maximum window limit!");
    }

    sp<IBinder> client = IInterface::asBinder(window);
    auto itState = mWindowMap.find(client);
    if (itState != mWindowMap.end()) {
        *_aidl_return = -1;
        WM_PROFILER_END();
        return Status::fromExceptionCode(1, "window already exist");
    }

    sp<IBinder> token = attrs.mToken;

    auto itToken = mTokenMap.find(token);
    if (itToken == mTokenMap.end()) {
        *_aidl_return = -1;
        WM_PROFILER_END();
        return Status::fromExceptionCode(1, "please add token firstly");
    }

    WindowToken* winToken = itToken->second;
    WindowState* win = new WindowState(this, window, winToken, attrs, visibility,
                                       outInputChannel != nullptr ? true : false);
    client->linkToDeath(mWindowDeathRecipient);
    mWindowMap.emplace(client, win);
    winToken->addWindow(win);

    if (outInputChannel != nullptr && attrs.hasInput()) {
        std::string name = genUniquePath(pid, "event");
        std::shared_ptr<InputDispatcher> inputDispatcher = win->createInputDispatcher(name);
        outInputChannel->copyFrom(inputDispatcher->getInputChannel());
    }

    *_aidl_return = 0;
    WM_PROFILER_END();

    return Status::ok();
}

Status WindowManagerService::removeWindow(const sp<IWindow>& window) {
    WM_PROFILER_BEGIN();

    FLOGI("[%d] window(%p)", IPCThreadState::self()->getCallingPid(), window.get());
    sp<IBinder> client = IInterface::asBinder(window);
    auto itState = mWindowMap.find(client);
    if (itState != mWindowMap.end()) {
        itState->second->removeIfPossible();
        mWindowMap.erase(client);
    } else {
        return Status::fromExceptionCode(1, "can't find winstate in map");
    }
    WM_PROFILER_END();

    return Status::ok();
}

Status WindowManagerService::relayout(const sp<IWindow>& window, const LayoutParams& attrs,
                                      int32_t requestedWidth, int32_t requestedHeight,
                                      int32_t visibility, SurfaceControl* outSurfaceControl,
                                      int32_t* _aidl_return) {
    WM_PROFILER_BEGIN();

    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "] window(%p) size(%" PRId32 "x%" PRId32 ")", pid, window.get(),
          requestedWidth, requestedHeight);

    *_aidl_return = -1;
    sp<IBinder> client = IInterface::asBinder(window);
    WindowState* win;
    auto it = mWindowMap.find(client);
    if (it != mWindowMap.end()) {
        win = it->second;
    } else {
        FLOGW("[%" PRId32 "] please add window firstly", pid);
        WM_PROFILER_END();
        return Status::fromExceptionCode(1, "please add window firstly");
    }

    bool visible = visibility == LayoutParams::WINDOW_VISIBLE ? true : false;
    win->destroySurfaceControl();

    if (visible) {
        if (attrs.mWidth != requestedWidth || attrs.mHeight != requestedHeight) {
            LayoutParams newAttrs = attrs;
            newAttrs.mWidth = requestedWidth;
            newAttrs.mHeight = requestedHeight;
            win->setLayoutParams(newAttrs);
        } else {
            win->setLayoutParams(attrs);
        }
        *_aidl_return = createSurfaceControl(outSurfaceControl, win);
    } else {
        outSurfaceControl = nullptr;
    }

    win->setVisibility(visibility);

    WM_PROFILER_END();
    return Status::ok();
}

Status WindowManagerService::isWindowToken(const sp<IBinder>& binder, bool* _aidl_return) {
    WM_PROFILER_BEGIN();

    auto it = mTokenMap.find(binder);
    if (it != mTokenMap.end()) {
        *_aidl_return = true;
    } else {
        *_aidl_return = false;
    }
    WM_PROFILER_END();
    FLOGI("result %s", (*_aidl_return) ? "success" : "failure");

    return Status::ok();
}

Status WindowManagerService::addWindowToken(const sp<IBinder>& token, int32_t type,
                                            int32_t displayId) {
    WM_PROFILER_BEGIN();
    int32_t pid = IPCThreadState::self()->getCallingPid();

    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        FLOGW("[%" PRId32 "] window token(%p) already exist", pid, token.get());
        return Status::fromExceptionCode(1, "window token already exist");
    } else {
        WindowToken* windToken = new WindowToken(this, token, type, displayId, pid);
        mTokenMap.emplace(token, windToken);
        FLOGI("[%" PRId32 "] add window token(%p) success", pid, token.get());
    }
    WM_PROFILER_END();
    return Status::ok();
}

Status WindowManagerService::removeWindowToken(const sp<IBinder>& token, int32_t displayId) {
    WM_PROFILER_BEGIN();

    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "] remove token(%p)", pid, token.get());

    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        it->second = nullptr;
        mTokenMap.erase(token);
    } else {
        return Status::fromExceptionCode(1, "can't find token in map");
    }
    WM_PROFILER_END();

    return Status::ok();
}

Status WindowManagerService::updateWindowTokenVisibility(const sp<IBinder>& token,
                                                         int32_t visibility) {
    WM_PROFILER_BEGIN();
    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "] update token(%p)'s visibility to %" PRId32 "", pid, token.get(),
          visibility);

    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        it->second->setClientVisibility(visibility);
    } else {
        FLOGI("[%" PRId32 "] can't find token %p in map", pid, token.get());
        return Status::fromExceptionCode(1, "can't find token in map");
    }
    WM_PROFILER_END();
    return Status::ok();
}

Status WindowManagerService::applyTransaction(const vector<LayerState>& state) {
    WM_PROFILER_BEGIN();
    for (const auto& layerState : state) {
        if (mWindowMap.find(layerState.mToken) != mWindowMap.end()) {
            mWindowMap[layerState.mToken]->applyTransaction(layerState);
        }
    }
    WM_PROFILER_END();
    return Status::ok();
}

Status WindowManagerService::requestVsync(const sp<IWindow>& window, VsyncRequest freq) {
    WM_PROFILER_BEGIN();
    FLOGD("%p freq:%d", window.get(), (int)freq);
    sp<IBinder> client = IInterface::asBinder(window);
    auto it = mWindowMap.find(client);

    if (it != mWindowMap.end()) {
        if (!it->second->scheduleVsync(freq)) {
            FLOGI("%p scheduleVsync %d for %p failure!", window.get(), (int)freq, it->second);
        }
    } else {
        WM_PROFILER_END();
        FLOGI("%p scheduleVsync %d for %p(not added)!", window.get(), (int)freq, it->second);
        return Status::fromExceptionCode(1, "can't find winstate in map");
    }
    WM_PROFILER_END();

    return Status::ok();
}

Status WindowManagerService::monitorInput(const sp<IBinder>& token, const ::std::string& name,
                                          int32_t displayId, InputChannel* outInputChannel) {
    int32_t pid = IPCThreadState::self()->getCallingPid();
    auto it = mInputMonitorMap.find(token);
    if (it != mInputMonitorMap.end()) {
        FLOGW("[%" PRId32 "] don't register input monitor repeatly!", pid);
        return Status::fromExceptionCode(1, "don't register input monitor repeatly");
    }

    std::string input_name = genUniquePath(pid, "monitor", name.c_str());
    auto dispatcher = InputDispatcher::create(input_name);
    if (dispatcher == nullptr) {
        return Status::fromExceptionCode(2, "monitor input is failure!");
    }

    mInputMonitorMap.emplace(token, dispatcher);
    outInputChannel->copyFrom(dispatcher->getInputChannel());

    FLOGI("for %" PRId32 "", dispatcher->getInputChannel().getEventFd());
    return Status::ok();
}

Status WindowManagerService::releaseInput(const sp<IBinder>& token) {
    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "]", pid);

    auto it = mInputMonitorMap.find(token);
    if (it != mInputMonitorMap.end()) {
        mInputMonitorMap.erase(token);
        return Status::ok();
    }
    return Status::fromExceptionCode(1, "no specified input monitor");
}

void WindowManagerService::doRemoveWindow(const sp<IWindow>& window) {
    sp<IBinder> binder = IInterface::asBinder(window);
    auto itState = mWindowMap.find(binder);
    if (itState != mWindowMap.end()) {
        delete itState->second;
        itState->second = nullptr;
        mWindowMap.erase(binder);
    }
}

void WindowManagerService::responseInput(const InputMessage* msg) {
    if (!msg) return;

    int ret = 0;
    for (const auto& [token, dispatcher] : mInputMonitorMap) {
        ret = dispatcher->sendMessage(msg);
        if (ret != 0) {
            FLOGW("dispatch input monitor for %" PRId32 " failure: %d",
                  dispatcher->getInputChannel().getEventFd(), ret);
        }
    }
}

bool WindowManagerService::responseVsync() {
    WM_PROFILER_BEGIN();

    VsyncRequest nextVsync = VsyncRequest::VSYNC_REQ_NONE;
    for (const auto& [key, state] : mWindowMap) {
        if (state->isVisible()) {
            VsyncRequest result = state->onVsync();
            if (result > nextVsync) {
                nextVsync = result;
            }
        }
    }

    if (nextVsync == VsyncRequest::VSYNC_REQ_NONE) {
        mContainer->enableVsync(false);
    }

    WM_PROFILER_END();
    return true;
}

int32_t WindowManagerService::createSurfaceControl(SurfaceControl* outSurfaceControl,
                                                   WindowState* win) {
    int32_t result = 0;

    // double buffer: Create shared memory
    int32_t size = win->getSurfaceSize();

    vector<BufferId> ids;
    for (int32_t i = 0; i < 2; i++) {
        BufferId id;
        if (!createSharedBuffer(size, &id)) {
            for (int32_t j = 0; j < (int)ids.size(); j++) {
                close(ids[j].mFd);
            }
            ids.clear();
            FLOGE("createSharedBuffer failed, clear buffer ids!");
            return -1;
        }
        ids.push_back(id);
    }
    std::shared_ptr<SurfaceControl> surfaceControl = win->createSurfaceControl(ids);

    if (surfaceControl != nullptr) {
        outSurfaceControl->copyFrom(*surfaceControl);
    }

    return result;
}

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
AnimEngineHandle WindowManagerService::getAnimEngine() {
    return mWinAnimEngine->getEngine();
}

std::string WindowManagerService::getAnimConfig(bool animMode, WindowState* win) {
    return (mAnimConfigMap.size() < 2)
            ? ((animMode) ? defaultEnterConfigJson : defaultExitConfigJson)
            : ((animMode) ? mAnimConfigMap[1] : mAnimConfigMap[2]);
}
#endif

} // namespace wm
} // namespace os
