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
#include <utils/Log.h>

#include "wm/WMService.h"

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
#include "WindowAnimEngine.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#endif
#include "../common/WindowUtils.h"
#include "GestureDetector.h"
#include "InputDispatcher.h"
#include "RootContainer.h"
#include "WindowState.h"
#include "WindowToken.h"
#include "wm/GestureDetectorState.h"
#include "wm/SurfaceControl.h"
#include "wm/VsyncRequestOps.h"

using ::android::IServiceManager;
using ::android::sp;
using ::android::String16;
using ::os::wm::IWindowManager;
using ::os::wm::WindowManagerService;

sp<IWindowManager> startWMService(sp<IServiceManager> sm,
                                  std::shared_ptr<::os::app::UvLoop> uvLooper) {
    if (!uvLooper) return nullptr;

    sp<WindowManagerService> wms = sp<WindowManagerService>::make(uvLooper);
    if (!wms->ready()) return nullptr;

    sm->addService(String16(WindowManagerService::name()), wms);
    return wms;
}

namespace os {
namespace wm {

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
static std::map<int, std::string> mAnimConfigMap;
static const std::string animConfigPath = "/etc/xms/window_anim_config.json";

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
                    "cubic-bezier",
                    0.3,
                    0.42, 
                    0.0, 
                    0.58, 
                    1.0
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
                    "cubic-bezier",
                    0.2,
                    0.42, 
                    0.0, 
                    0.58, 
                    1.0
                ]
            }
        })";

static const std::string defaultSystemWindowEnterConfigJson =
        R"({
            "fromState": {
                "opacity": 0.0,
                "y": -480.0
            },
            "toState": {
                "opacity": 1.0,
                "y": 0.0
            },
            "config": {
                "ease": [
                    "cubic-bezier",
                    0.3,
                    0.42, 
                    0.0, 
                    0.58, 
                    1.0
                ]
            }
        })";

static const std::string defaultSystemWindowExitConfigJson =
        R"({
            "fromState": {
                "opacity": 1.0,
                "y": 0.0
            },
            "toState": {
                "opacity": 0.0,
                "y": -480.0
            },
            "config": {
                "ease": [
                    "cubic-bezier",
                    0.3,
                    0.42, 
                    0.0, 
                    0.58, 
                    1.0
                ]
            }
        })";

static int parseAnimJsonFile(const char* filename) {
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

/* limited by mq_open */
#define MQ_PATH_MAXLEN 50
static inline std::string genUniquePath(bool needpath, int32_t pid, const std::string& prefix,
                                        const std::string name = "") {
    /* xms:bq-300-1234567890 or /var/run/xms:event-301-2345167890 */

    std::string path = (needpath ? "/var/run/xms:" : "xms:") + prefix;
    path += "-" + std::to_string(pid) + "-" +
            (name.size() > 0 ? name : std::to_string(std::rand()));

    FLOGI("'%s', length is %d", path.c_str(), (int)path.size());
    return path.size() <= MQ_PATH_MAXLEN ? path : path.substr(0, MQ_PATH_MAXLEN);
}

void WindowManagerService::WindowDeathRecipient::binderDied(const wp<IBinder>& who) {
    FLOGW("window binder died");
    auto key = who.promote();
    auto it = mService->mWindowMap.find(key);
    if (it != mService->mWindowMap.end()) {
        it->second->removeIfPossible();
    }
}

WindowManagerService::WindowManagerService(std::shared_ptr<::os::app::UvLoop> uvLooper)
      : mUvLooper(uvLooper),
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
        mWinAnimEngine(nullptr),
#endif
        mGestureDetector(mUvLooper) {
    FLOGI("WMS init");
    mContainer = new RootContainer(this, mUvLooper->get());
    DisplayInfo disp_info;
    mContainer->getDisplayInfo(&disp_info);
    mGestureDetector.setDisplayInfo(&disp_info);

    if (!ready()) return;

    mWindowDeathRecipient = sp<WindowDeathRecipient>::make(this);
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    mWinAnimEngine = new WindowAnimEngine();
    int ret = parseAnimJsonFile(animConfigPath.c_str());
    if (ret < 0) {
        ALOGE("Failed to parse file %s, use default animation config", animConfigPath.c_str());
    }
#endif
}

WindowManagerService::~WindowManagerService() {
    mInputMonitorMap.clear();
    if (mContainer) delete mContainer;
    mWindowDeathRecipient = nullptr;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mWinAnimEngine) delete mWinAnimEngine;
    mAnimConfigMap.clear();
#endif
}

bool WindowManagerService::ready() {
    return mContainer->ready();
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
        FLOGE("failure, exceed maximum window limit!");
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

    shared_ptr<WindowToken> winToken = nullptr;
    if (attrs.mToken != nullptr) {
        auto itToken = mTokenMap.find(attrs.mToken);
        if (itToken != mTokenMap.end()) {
            winToken = itToken->second;
        }
    }

    if (winToken == nullptr) {
        if (attrs.mType == LayoutParams::TYPE_APPLICATION) {
            *_aidl_return = -1;
            WM_PROFILER_END();
            return Status::fromExceptionCode(1, "please add token firstly");
        } else {
            FLOGI("for non-application, create token automatically");
            sp<IBinder> token = new BBinder();
            winToken = std::make_shared<WindowToken>(this, token, attrs.mType, displayId, pid);
            mTokenMap.emplace(token, winToken);
        }
    }

    WindowState* win = new WindowState(this, window, winToken, attrs, visibility,
                                       outInputChannel != nullptr ? true : false);
    client->linkToDeath(mWindowDeathRecipient);
    mWindowMap.emplace(client, win);
    winToken->addWindow(win);

    if (outInputChannel != nullptr && attrs.hasInput()) {
        std::string name = genUniquePath(true, pid, "event");
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

    *_aidl_return = 0;
    sp<IBinder> client = IInterface::asBinder(window);
    WindowState* win;
    auto it = mWindowMap.find(client);
    if (it != mWindowMap.end()) {
        win = it->second;
    } else {
        *_aidl_return = -1;
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
        if (*_aidl_return != 0) {
            FLOGE("failure, cann't create valid surface!");
            outSurfaceControl = nullptr;
        }
    } else {
        outSurfaceControl = nullptr;
    }

    win->setVisibility(visibility);

    WM_PROFILER_END();
    return *_aidl_return == 0
            ? Status::ok()
            : Status::fromExceptionCode(2, "now no valid surface, please retry it!");
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
        auto winToken = std::make_shared<WindowToken>(this, token, type, displayId, pid);
        mTokenMap.emplace(token, winToken);
        FLOGI("[%" PRId32 "] add window token(%p) success", pid, token.get());
    }
    WM_PROFILER_END();
    return Status::ok();
}

bool WindowManagerService::removeWindowTokenInner(sp<IBinder>& token) {
    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        mTokenMap.erase(token);
        return true;
    }
    return false;
}

Status WindowManagerService::removeWindowToken(const sp<IBinder>& token, int32_t displayId) {
    WM_PROFILER_BEGIN();

    int32_t pid = IPCThreadState::self()->getCallingPid();
    FLOGI("[%" PRId32 "] remove token(%p)", pid, token.get());

    auto it = mTokenMap.find(token);
    if (it != mTokenMap.end()) {
        it->second->removeIfPossible();
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

Status WindowManagerService::requestVsync(const sp<IWindow>& window, VsyncRequest vreq) {
    WM_PROFILER_BEGIN();
    FLOGD("%p vreq=%s", window.get(), VsyncRequestToString(vreq));
    sp<IBinder> client = IInterface::asBinder(window);
    auto it = mWindowMap.find(client);

    if (it != mWindowMap.end()) {
        if (!it->second->scheduleVsync(vreq)) {
            FLOGD("%p duplicate vreq=%s for %p!", window.get(), VsyncRequestToString(vreq),
                  it->second);
        }
    } else {
        WM_PROFILER_END();
        FLOGI("%p vreq=%s for %p(not added)!", window.get(), VsyncRequestToString(vreq),
              it->second);
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

    std::string input_name = genUniquePath(true, pid, "monitor", name);
    auto dispatcher = InputDispatcher::create(input_name);
    if (dispatcher == nullptr) {
        return Status::fromExceptionCode(2, "monitor input is failure!");
    }

    mInputMonitorMap.emplace(token, dispatcher);
    outInputChannel->copyFrom(dispatcher->getInputChannel());

    FLOGI("for %d", dispatcher->getInputChannel().getEventFd());
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

void WindowManagerService::postWindowRemoveCleanup(WindowState* state) {
    mUvLooper->postTask([this, state]() {
        sp<IBinder> binder = IInterface::asBinder(state->getClient());
        auto token = state->getToken();

        if (token && token.get()) token->removeWindow(state);

        auto itState = mWindowMap.find(binder);
        if (itState != mWindowMap.end()) {
            itState->first->unlinkToDeath(mWindowDeathRecipient);
            delete itState->second;
            mWindowMap.erase(binder);
        }

        if (token && token.get() && token->isEmpty() && !token->isPersistOnEmpty()) {
            token->removeIfPossible();
        }
    });
}

bool WindowManagerService::responseInput(InputMessage* msg) {
    if (!msg) return false;

    /* sync: system gesture recognize */
    msg->pointer.gesture_state = mGestureDetector.recognizeGesture(msg);
    bool has_gesture = msg->pointer.gesture_state != 0;

    /* async: input monitor notification */
    int ret = 0;
    for (const auto& [token, dispatcher] : mInputMonitorMap) {
        ret = dispatcher->sendMessage(msg);
        if (ret != 0) {
            FLOGW("dispatch input monitor for %d failure: %d",
                  dispatcher->getInputChannel().getEventFd(), ret);
        }
    }
    return has_gesture;
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
    vector<BufferId> ids;
    int32_t pid = IPCThreadState::self()->getCallingPid();
    int32_t bufferCount = 2;

#ifdef CONFIG_ENABLE_WINDOW_TRIPLE_BUFFER
    bufferCount = 3;
#endif

    for (int32_t i = 0; i < bufferCount; i++) {
        BufferId id;
        std::string bufferPath = genUniquePath(false, pid, "bq");
        int32_t bufferKey = std::rand();
        id = {bufferPath, bufferKey, -1};
        ids.push_back(id);
    }
    std::shared_ptr<SurfaceControl> surfaceControl = win->createSurfaceControl(ids);

    if (!surfaceControl->isValid()) {
        outSurfaceControl = nullptr;
        return -1;
    }

    if (surfaceControl != nullptr) {
        outSurfaceControl->copyFrom(*surfaceControl);
    }

    return 0;
}

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
AnimEngineHandle WindowManagerService::getAnimEngine() {
    return mWinAnimEngine->getEngine();
}

std::string WindowManagerService::getAnimConfig(bool animMode, WindowState* win) {
    return (mAnimConfigMap.size() < 2)
            ? ((animMode) ? ((win->getToken()->getType() == LayoutParams::TYPE_SYSTEM_WINDOW)
                                     ? defaultSystemWindowEnterConfigJson
                                     : defaultEnterConfigJson)
                          : ((win->getToken()->getType() == LayoutParams::TYPE_SYSTEM_WINDOW)
                                     ? defaultSystemWindowExitConfigJson
                                     : defaultExitConfigJson))
            : ((animMode) ? mAnimConfigMap[1] : mAnimConfigMap[2]);
}
#endif

} // namespace wm
} // namespace os
