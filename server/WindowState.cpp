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

#define LOG_TAG "WMS:Window"

#include "WindowState.h"

#include <sys/mman.h>
#include <utils/RefBase.h>

#include <map>

#include "../common/WindowUtils.h"
#include "RootContainer.h"
#include "WindowManagerService.h"
#include "wm/LayerState.h"
#include "wm/VsyncRequestOps.h"
namespace os {
namespace wm {

static inline void* getLayerByType(WindowManagerService* service, int type) {
    switch (type) {
        case LayoutParams::TYPE_DIALOG: {
            return service->getRootContainer()->getSysLayer();
        }
        case LayoutParams::TYPE_SYSTEM_WINDOW:
        case LayoutParams::TYPE_TOAST: {
            return service->getRootContainer()->getTopLayer();
        }
        case LayoutParams::TYPE_APPLICATION:
        default: {
            return service->getRootContainer()->getDefLayer();
        }
    }
}

WindowState::WindowState(WindowManagerService* service, const sp<IWindow>& window,
                         std::shared_ptr<WindowToken> token, const LayoutParams& params,
                         int32_t visibility, bool enableInput)
      : mClient(window),
        mToken(token),
        mService(service),
        mFrameReq(0),
        mFlags(0),
        mNeedInput(enableInput),
        mSurfaceControl(nullptr),
        mInputDispatcher(nullptr),
        mHasSurface(false),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE) {
    mAttrs = params;

    Rect rect(params.mX, params.mY, params.mX + params.mWidth, params.mY + params.mHeight);
    mNode = new WindowNode(this, getLayerByType(mService, mToken->getType()), rect, enableInput,
                           mAttrs.mFormat);

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    mFrameWaiting = true;
    mAnimRunning = false;
    mWinAnimator = new WindowAnimator(mService->getAnimEngine(), mNode->getWidget());
#endif
}

WindowState::~WindowState() {
    mClient = nullptr;
    if (mNode) delete mNode;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mWinAnimator) delete mWinAnimator;
#endif
    mToken = nullptr;
    FLOGI("done");
}

void WindowState::setVisibility(int32_t visibility) {
    mNode->setVisibility(visibility);
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (xmsLiteMode() && visibility == LayoutParams::WINDOW_VISIBLE) windowTransition(true);
#endif
}

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
bool WindowState::windowTransition(bool in) {
    /* Check if window transition is enabled for this window */
    if (mAttrs.mWindowTransitionState != LayoutParams::WINDOW_TRANSITION_ENABLE) {
        FLOGD("[%d] Window transition not enabled", mToken->getClientPid());
        return false;
    }

    /* Validate animation state before proceeding */
    if (mAnimRunning) {
        FLOGW("[%d] Animation already running, cannot start new transition",
              mToken->getClientPid());
        return false;
    }

    if (in) {
        /* Handle in-transition (window appearing) */
        if (!mFrameWaiting) {
            return false;
        }

        mFrameWaiting = false;
        mNode->resetOpaque();
        FLOGD("[%d] Starting in-transition, reset window opacity", mToken->getClientPid());
    } else {
        /* Handle out-transition (window disappearing) */
        if (mFrameWaiting) {
            return false;
        }
        FLOGD("[%d] Starting out-transition", mToken->getClientPid());
    }

    /* Get animation configuration and start animation */
    std::string animConfig = mService->getAnimConfig(in, this);
    if (animConfig.empty()) {
        FLOGE("[%d] Failed to get animation configuration", mToken->getClientPid());
        return false;
    }

    mAnimRunning = true;
    FLOGI("[%d] Starting %s transition animation", mToken->getClientPid(), in ? "in" : "out");

    int result = mWinAnimator->startAnimation(animConfig, [this](WindowAnimStatus status) {
        this->onAnimationFinished(status);
    });

    if (result != 0) {
        FLOGE("[%d] Failed to start animation, error code: %d", mToken->getClientPid(), result);
        mAnimRunning = false;
        return false;
    }

    return true;
}
#endif

void WindowState::sendAppVisibilityToClients(int32_t visibility) {
    int32_t oldVisibility = mNode->getVisibility();
    if (oldVisibility == visibility) {
        FLOGI("[%d] token=%p, no changed visibility %s", mToken->getClientPid(), mToken.get(),
              LayoutParams::visibilityToName(visibility));
        return;
    }

    WM_PROFILER_BEGIN();

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mAnimRunning) {
        mWinAnimator->cancelAnimation();
    }
#endif
    mNode->syncClientVisibility(visibility);
    bool visible = visibility == LayoutParams::WINDOW_VISIBLE ? true : false;

    FLOGI("[%d] update token=%p visibility to %s", mToken->getClientPid(), mToken.get(),
          LayoutParams::visibilityToName(visibility));

    if (!visible) {
        if (xmsLiteMode()) {
            /* to HOLD */
            if (visibility == LayoutParams::WINDOW_HOLD) {
                WM_PROFILER_END();
                return;
            }
        } else {
            scheduleVsync(VsyncRequest::VSYNC_REQ_NONE);
        }

        /* VISIBLE to HOLD/GONE, HOLD to GONE */
        if (!isVisible()) {
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
            if (!windowTransition(false)) {
                mClient->dispatchAppVisibility(visible);
            }
#else
            mClient->dispatchAppVisibility(visible);
#endif
        }
    } else {
        /* to VISIBLE */
        if (!xmsLiteMode()) {
            scheduleVsync(mVsyncRequest != VsyncRequest::VSYNC_REQ_NONE
                                  ? mVsyncRequest
                                  : VsyncRequest::VSYNC_REQ_SINGLE);
        }

        mClient->dispatchAppVisibility(visible);
    }

    WM_PROFILER_END();
}

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
void WindowState::onAnimationFinished(WindowAnimStatus status) {
    if (status == WINDOW_ANIM_STATUS_FINISHED) {
        mAnimRunning = false;
        FLOGI("[%d] token=%p", mToken->getClientPid(), mToken.get());

        if (!isVisible() && !(mFlags & WS_CLIENT_EXITED)) {
            mClient->dispatchAppVisibility(false);
        }

        if (mFlags & WS_ALLOW_REMOVING) {
            removeIfPossible();
        }
    }
}
#endif

void WindowState::removeIfPossible() {
    mFlags |= WS_ALLOW_REMOVING;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (!mAnimRunning)
#endif
    {
        removeImmediately();
    }
}

void WindowState::removeImmediately() {
    FLOGI("[%d]", mToken->getClientPid());

    if (mFlags & WS_REMOVED) return;

    mFlags |= WS_REMOVED;

    if (xmsLiteMode()) {
        mService->postWindowRemoveCleanup(this);
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
        mFrameWaiting = true;
#endif
        return;
    }

    scheduleVsync(VsyncRequest::VSYNC_REQ_NONE);
    destroySurfaceControl();
    if (mInputDispatcher.get() != nullptr) {
        mInputDispatcher->release();
    }

    mService->postWindowRemoveCleanup(this);
}

void WindowState::setLayoutParams(LayoutParams attrs) {
    if (!xmsLiteMode() && mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        FLOGW("shouldn't update layout configuration when surface is valid!");
        return;
    }

    mAttrs = attrs;
    Rect rect(attrs.mX, attrs.mY, attrs.mX + attrs.mWidth, attrs.mY + attrs.mHeight);
    mNode->setRect(rect);
}

uint32_t WindowState::getSurfaceSize() {
    return mNode->getSurfaceSize();
}

bool WindowState::isVisible() {
    return mNode->getVisibility() != LayoutParams::WINDOW_GONE;
}

void* WindowState::getClientScreen() {
    return mNode->getClientScreen();
}

/*========== only for multi-instance mode ==========*/
std::shared_ptr<BufferConsumer> WindowState::getBufferConsumer() {
    if (xmsLiteMode()) return nullptr;

    if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferConsumer>(mSurfaceControl->bufferQueue());
    }
    return nullptr;
}

std::shared_ptr<InputDispatcher> WindowState::createInputDispatcher(const std::string& name) {
    if (xmsLiteMode()) return nullptr;

    if (mInputDispatcher != nullptr) {
        FLOGE("input dispatcher has existed, needn't create again.");
        return nullptr;
    }
    mInputDispatcher = InputDispatcher::create(name);
    return mInputDispatcher;
}

bool WindowState::sendInputMessage(const InputMessage* ie) {
    if (xmsLiteMode()) return false;
    if (mInputDispatcher != nullptr) return mInputDispatcher->sendMessage(ie);
    return false;
}

std::shared_ptr<SurfaceControl> WindowState::createSurfaceControl(const std::vector<BufferId>& ids,
                                                                  const std::string& fmqName) {
    if (xmsLiteMode()) return nullptr;

    WM_PROFILER_BEGIN();

    destroySurfaceControl();
    setHasSurface(false);

    sp<IBinder> handle = sp<BBinder>::make();
    mSurfaceControl =
            std::make_shared<SurfaceControl>(IInterface::asBinder(mClient), handle, mAttrs.mWidth,
                                             mAttrs.mHeight, mAttrs.mFormat, getSurfaceSize());
    mSurfaceControl->getFMQ().setName(fmqName);
    mSurfaceControl->initBufferIds(ids);
    initSurfaceBuffer(mSurfaceControl, true);
    /* should update after init fmq */
    mSurfaceControl->getFMQ().updateClientRespSeq(mFrameReq);

    std::shared_ptr<BufferConsumer> buffConsumer =
            std::make_shared<BufferConsumer>(mSurfaceControl);
    mSurfaceControl->setBufferQueue(buffConsumer);

    setHasSurface(true);
    WM_PROFILER_END();

    return mSurfaceControl;
}

void WindowState::destroySurfaceControl() {
    if (xmsLiteMode()) {
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
        mFrameWaiting = true;
#endif
        return;
    }

    if (mHasSurface) {
        setHasSurface(false);
        if (mNode != nullptr) {
            FLOGI("updateBuffer NULLPTR");
            mNode->updateBuffer(nullptr, nullptr, 0);
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
            mFrameWaiting = true;
#endif
        }
        uninitSurfaceBuffer(mSurfaceControl);
        mSurfaceControl.reset();
    }
    FLOGI("done");
}

void WindowState::applyTransaction(LayerState layerState) {
    if (xmsLiteMode()) return;

    FLOGD("[%d] seq=%" PRIu32 "", mToken->getClientPid(), layerState.mSeq);
    WM_PROFILER_BEGIN();

    BufferItem* buffItem = nullptr;
    Rect* rect = nullptr;
    if (layerState.mFlags & LayerState::LAYER_POSITION_CHANGED) {
    }

    if (layerState.mFlags & LayerState::LAYER_ALPHA_CHANGED) {
    }

    if (layerState.mFlags & LayerState::LAYER_BUFFER_CHANGED) {
        std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
        if (consumer == nullptr) {
            WM_PROFILER_END();
            return;
        }
        buffItem = consumer->syncQueuedState(layerState.mBufferKey);
    }

    if (layerState.mFlags & LayerState::LAYER_BUFFER_CROP_CHANGED) {
        rect = &layerState.mBufferCrop;
    }
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mFrameWaiting &&
        (mAttrs.mWindowTransitionState == LayoutParams::WINDOW_TRANSITION_ENABLE)) {
        windowTransition(true);
    }

    if (mAnimRunning && (buffItem == nullptr)) {
        FLOGW("[%d] animation is running, drop the null buffer data", mToken->getClientPid());
        return;
    }
#endif

    mNode->updateBuffer(buffItem, rect, layerState.mSeq);
    WM_PROFILER_END();
}

bool WindowState::scheduleVsync(VsyncRequest vsyncReq) {
    if (xmsLiteMode()) return false;

    mService->getRootContainer()->enableVsync(true);

    if (mVsyncRequest == vsyncReq) {
        return false;
    }

    /* observer for animation */
    if (vsyncReq == VsyncRequest::VSYNC_REQ_PERIODIC ||
        mVsyncRequest == VsyncRequest::VSYNC_REQ_PERIODIC)
        FLOGW("[%d] request vreq=%s", mToken->getClientPid(), VsyncRequestToString(vsyncReq));

    mVsyncRequest = vsyncReq;

    return true;
}

VsyncRequest WindowState::onVsync() {
    if (xmsLiteMode()) return mVsyncRequest;

    if (mVsyncRequest == VsyncRequest::VSYNC_REQ_NONE) {
        return mVsyncRequest;
    }
    WM_PROFILER_BEGIN();

    /*only for periodic vsync*/
    if (mVsyncRequest == VsyncRequest::VSYNC_REQ_PERIODIC) {
        if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
            uint32_t lastResp = mSurfaceControl->getFMQ().getClientRespSeq();
            if (mFrameReq != lastResp) {
                FLOGD("[%d], send vsync response, frame seq=%" PRIu32 ", last resp seq=%" PRIu32 "",
                      mToken->getClientPid(), mFrameReq, lastResp);

                const uint32_t threshold = mSurfaceControl->getFMQ().getQueueCaps();
                if ((mFrameReq - lastResp) >= threshold) {
                    /*only warning once*/
                    if (mFlags & WS_CLIENT_TIMEOUT) {
                        WM_PROFILER_END();
                        return mVsyncRequest;
                    }

                    mFlags |= WS_CLIENT_TIMEOUT;
                    FLOGW("[%d], send vsync response, response timeout! (frame seq=%" PRIu32
                          ", last resp seq=%" PRIu32 ", threshold=%" PRIu32 ")",
                          mToken->getClientPid(), mFrameReq, lastResp, threshold);
                    WM_PROFILER_END();
                    return mVsyncRequest;
                }
            }

            if (mFlags & WS_CLIENT_TIMEOUT) {
                mFlags &= ~WS_CLIENT_TIMEOUT;
                FLOGW("[%d], send vsync response, vsync restored (frame seq=%" PRIu32 ")",
                      mToken->getClientPid(), mFrameReq);
            }
        }
    }

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    mClient->onFrame(++mFrameReq);
    if (mFrameReq == UINT32_MAX) mFrameReq = 0;

    WM_PROFILER_END();

    return mVsyncRequest;
}

BufferItem* WindowState::acquireBuffer() {
    if (xmsLiteMode()) return nullptr;

    FLOGD("acquire");
    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer == nullptr) {
        return nullptr;
    }

    return consumer->acquireBuffer();
}

bool WindowState::releaseBuffer(BufferItem* buffer) {
    if (xmsLiteMode()) return false;

    FLOGD("release");

    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer == nullptr) {
        return false;
    }

    if (consumer && consumer->releaseBuffer(buffer) && mClient) {
        WM_PROFILER_BEGIN();

        if (!mSurfaceControl->getFMQ().write(&(buffer->mKey))) {
            FLOGE("Failed to relase bufKey=%" PRId32 "", buffer->mKey);
            /* fallback non-fmq */
            mClient->bufferReleased(buffer->mKey);
        } else {
            FLOGD("success to relase bufKey=%" PRId32 "", buffer->mKey);
        }
        WM_PROFILER_END();
        return true;
    }
    return false;
}

} // namespace wm
} // namespace os
