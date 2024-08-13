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
        mInputDispatcher(nullptr),
        mVsyncRequest(VsyncRequest::VSYNC_REQ_NONE),
        mFrameReq(0),
        mHasSurface(false),
        mFlags(0),
        mNeedInput(enableInput) {
    mAttrs = params;
    mVisibility = visibility;

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
    FLOGI("%p", this);
    mClient = nullptr;
    if (mNode) delete mNode;
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mWinAnimator) delete mWinAnimator;
#endif
    mToken = nullptr;
}

std::shared_ptr<BufferConsumer> WindowState::getBufferConsumer() {
    if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        return std::static_pointer_cast<BufferConsumer>(mSurfaceControl->bufferQueue());
    }
    return nullptr;
}

std::shared_ptr<InputDispatcher> WindowState::createInputDispatcher(const std::string& name) {
    if (mInputDispatcher != nullptr) {
        FLOGE("%p input dispatcher has existed, needn't create again.", this);
        return nullptr;
    }
    mInputDispatcher = InputDispatcher::create(name);
    return mInputDispatcher;
}

bool WindowState::sendInputMessage(const InputMessage* ie) {
    if (mInputDispatcher != nullptr) return mInputDispatcher->sendMessage(ie);
    return false;
}

void WindowState::setVisibility(int32_t visibility) {
    mVisibility = visibility;
    FLOGI("%p [%d] visibility=%" PRId32 " (0:visible, 1:hold, 2:gone)", this,
          mToken->getClientPid(), visibility);
    if (mNeedInput) mNode->enableInput(visibility == LayoutParams::WINDOW_VISIBLE);
}

void WindowState::sendAppVisibilityToClients(int32_t visibility) {
    if (!isVisible() && visibility == LayoutParams::WINDOW_GONE) return;
    WM_PROFILER_BEGIN();

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
    if (mAnimRunning) {
        mWinAnimator->cancelAnimation();
    }
#endif
    setVisibility(visibility);
    bool visible = visibility == LayoutParams::WINDOW_VISIBLE ? true : false;

    FLOGI("%p [%d] token=%p update visibility to %s", this, mToken->getClientPid(), mToken.get(),
          visible ? "visible" : "invisible");

    if (!visible) {
        scheduleVsync(VsyncRequest::VSYNC_REQ_NONE);
        if (!isVisible()) {
#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
            if (mAttrs.mWindowTransitionState == LayoutParams::WINDOW_TRANSITION_ENABLE) {
                mAnimRunning = true;
                mWinAnimator->startAnimation(mService->getAnimConfig(false, this),
                                             [this](WindowAnimStatus status) {
                                                 this->onAnimationFinished(status);
                                             });
            } else {
                mClient->dispatchAppVisibility(visible);
            }
#else
            mClient->dispatchAppVisibility(visible);
#endif
        }
    } else {
        scheduleVsync(mVsyncRequest != VsyncRequest::VSYNC_REQ_NONE
                              ? mVsyncRequest
                              : VsyncRequest::VSYNC_REQ_SINGLE);
        mClient->dispatchAppVisibility(visible);
    }
    WM_PROFILER_END();
}

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
void WindowState::onAnimationFinished(WindowAnimStatus status) {
    if (status == WINDOW_ANIM_STATUS_FINISHED) {
        mAnimRunning = false;
        FLOGI("%p [%d] token=%p, visibility=%" PRId32 "", this, mToken->getClientPid(),
              mToken.get(), mVisibility);
        if (mVisibility != LayoutParams::WINDOW_VISIBLE) {
            mClient->dispatchAppVisibility(false);
        }
        if (mFlags & WS_ALLOW_REMOVING) {
            removeIfPossible();
        }
    }
}
#endif

std::shared_ptr<SurfaceControl> WindowState::createSurfaceControl(const std::vector<BufferId>& ids,
                                                                  const std::string& fmqName) {
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

    std::shared_ptr<BufferConsumer> buffConsumer =
            std::make_shared<BufferConsumer>(mSurfaceControl);
    mSurfaceControl->setBufferQueue(buffConsumer);

    setHasSurface(true);
    WM_PROFILER_END();

    return mSurfaceControl;
}

void WindowState::destroySurfaceControl() {
    FLOGI("%p", this);
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
}

void WindowState::applyTransaction(LayerState layerState) {
    FLOGD("%p [%d] seq=%" PRIu32 "", this, mToken->getClientPid(), layerState.mSeq);
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
        mFrameWaiting = false;
        if (mAnimRunning) {
            mWinAnimator->cancelAnimation();
        }
        mAnimRunning = true;
        mNode->resetOpaque();
        mWinAnimator->startAnimation(mService->getAnimConfig(true, this),
                                     [this](WindowAnimStatus status) {
                                         this->onAnimationFinished(status);
                                     });
    }

    if (mAnimRunning && (buffItem == nullptr)) {
        FLOGW("%p [%d] animation is running, drop the null buffer data", this,
              mToken->getClientPid());
        return;
    }

#endif

    mNode->updateBuffer(buffItem, rect, layerState.mSeq);
    WM_PROFILER_END();
}

bool WindowState::scheduleVsync(VsyncRequest vsyncReq) {
    mService->getRootContainer()->enableVsync(true);

    if (mVsyncRequest == vsyncReq) {
        return false;
    }

    /* observer for animation */
    if (vsyncReq == VsyncRequest::VSYNC_REQ_PERIODIC ||
        mVsyncRequest == VsyncRequest::VSYNC_REQ_PERIODIC)
        FLOGW("%p [%d] request vreq=%s", this, mToken->getClientPid(),
              VsyncRequestToString(vsyncReq));

    mVsyncRequest = vsyncReq;

    return true;
}

VsyncRequest WindowState::onVsync() {
    if (mVsyncRequest == VsyncRequest::VSYNC_REQ_NONE) {
        return mVsyncRequest;
    }
    WM_PROFILER_BEGIN();

    mVsyncRequest = nextVsyncState(mVsyncRequest);
    mClient->onFrame(++mFrameReq);

    FLOGI("%p [%d] vreq=%s send vsync(seq=%" PRIu32 ") to client!", this, mToken->getClientPid(),
          VsyncRequestToString(mVsyncRequest), mFrameReq);

    if (mFrameReq == UINT32_MAX) mFrameReq = 0;

    WM_PROFILER_END();

    return mVsyncRequest;
}

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
    FLOGI("%p", this);

    if (mFlags & WS_REMOVED) return;

    mFlags |= WS_REMOVED;

    scheduleVsync(VsyncRequest::VSYNC_REQ_NONE);
    destroySurfaceControl();
    if (mInputDispatcher.get() != nullptr) {
        mInputDispatcher->release();
    }

    mService->postWindowRemoveCleanup(this);
}

BufferItem* WindowState::acquireBuffer() {
    FLOGD("%p", this);

    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer == nullptr) {
        return nullptr;
    }
    return consumer->acquireBuffer();
}

bool WindowState::releaseBuffer(BufferItem* buffer) {
    FLOGD("%p", this);

    std::shared_ptr<BufferConsumer> consumer = getBufferConsumer();
    if (consumer == nullptr) {
        return false;
    }

    if (consumer && consumer->releaseBuffer(buffer) && mClient) {
        WM_PROFILER_BEGIN();

        if (!mSurfaceControl->getFMQ().write(&(buffer->mKey))) {
            FLOGE("%p Failed to relase bufKey=%" PRId32 "", this, buffer->mKey);
            /* fallback non-fmq */
            mClient->bufferReleased(buffer->mKey);
        } else {
            FLOGD("%p success to relase bufKey=%" PRId32 "", this, buffer->mKey);
        }
        WM_PROFILER_END();
        return true;
    }
    return false;
}

void WindowState::setLayoutParams(LayoutParams attrs) {
    if (mSurfaceControl != nullptr && mSurfaceControl->isValid()) {
        FLOGW("%p shouldn't update layout configuration when surface is valid!", this);
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
    return mVisibility != LayoutParams::WINDOW_GONE ? true : false;
}

} // namespace wm
} // namespace os
