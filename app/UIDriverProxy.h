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

#include "../common/FrameMetaInfo.h"
#include "BaseWindowDefault.h"
#include "wm/BufferQueue.h"
#include "wm/InputMessage.h"
#include "wm/InputMonitor.h"
#include "wm/Rect.h"

namespace os {
namespace wm {

class BaseWindow;

/**
 * @class UIDriverProxy
 * @brief Abstract base class for UI Driver Proxies.
 *
 * This class defines the interface that all UI driver proxies
 * must implement. It includes methods for drawing frames,
 * handling input events, and managing visibility states for a
 * user interface.
 */
class UIDriverProxy {
public:
    UIDriverProxy(std::shared_ptr<BaseWindowDefault> win);

    virtual ~UIDriverProxy();

    /**
     * @brief Retrieves the root element of the UI.
     *
     * This method must be implemented by derived classes to return
     * the root object associated with the UI driver.
     *
     * @return A pointer to the root object.
     */
    virtual void* getRoot() = 0;

    /**
     * @brief Retrieves the window element associated with the UI driver.
     *
     * This method must be implemented by derived classes to return
     * the window object.
     *
     * @return A pointer to the window object.
     */
    virtual void* getWindow() = 0;

    /**
     * @brief Draws a frame using the provided buffer item.
     *
     * @param item A pointer to the BufferItem to be drawn.
     */
    virtual void drawFrame(BufferItem* item);

    /**
     * @brief Finishes the drawing operation.
     *
     * This method checks if the drawing operation is complete.
     *
     * @return True if the drawing operation is complete, false otherwise.
     */
    bool finishDrawing();

    /**
     * @brief Invalidates the current UI, requesting a refresh.
     *
     * @param periodic True if the invalidation is periodic.
     * @return True if invalidation is successful, false otherwise.
     */
    bool onInvalidate(bool periodic);

    /**
     * @brief Called when a buffer is dequeued for rendering.
     *
     * @return A pointer to the dequeued buffer.
     */
    virtual void* onDequeueBuffer();

    /**
     * @brief Queues a buffer for rendering.
     *
     * @return True if the buffer is successfully queued, false otherwise.
     */
    bool onQueueBuffer();

    /**
     * @brief Cancels the currently queued buffer.
     */
    void onCancelBuffer();

    /**
     * @brief Sets the crop rectangle for the UI.
     *
     * @param rect Reference to a Rect object defining the crop area.
     */
    void onRectCrop(Rect& rect);

    /**
     * @brief Retrieves the current cropping rectangle.
     *
     * @return Pointer to the current cropping rectangle.
     */
    Rect* rectCrop();

    /**
     * @brief Retrieves the current BufferItem.
     *
     * @return Pointer to the current BufferItem.
     */
    BufferItem* getBufferItem() {
        return mBufferItem;
    }

    /**
     * @brief Handles an input event.
     *
     * This method must be implemented by derived classes to handle
     * input events appropriately.
     *
     * @return True if the event is handled, false otherwise.
     */
    virtual void handleEvent() = 0;

    /**
     * @brief Reads an input event from the message queue.
     *
     * @param message Pointer to the InputMessage to be read.
     * @return True if the event is successfully read, false otherwise.
     */
    bool readEvent(InputMessage* message);

    /**
     * @brief Sets the input monitor for the UI driver.
     *
     * @param monitor Pointer to the InputMonitor instance.
     */
    bool checkInput();
    virtual void setInputMonitor(InputMonitor* monitor);

    /**
     * @brief Retrieves the associated InputMonitor.
     *
     * @return Pointer to the associated InputMonitor.
     */
    InputMonitor* getInputMonitor() {
        return mInputMonitor;
    }

    /**
     * @brief Updates the resolution of the UI.
     *
     * @param width New width for the UI.
     * @param height New height for the UI.
     * @param format New color format for the UI.
     */
    virtual void updateResolution(int32_t width, int32_t height, uint32_t format);

    /**
     * @brief Enumeration for buffer update states.
     */
    enum { UIP_BUFFER_UPDATE = 1, UIP_BUFFER_RECT_UPDATE = 2 };

    /**
     * @brief Sets an event listener for UI events.
     *
     * @param listener Pointer to the WindowEventListener.
     */
    void setEventListener(WindowEventListener* listener) {
        mEventListener = listener;
    }

    /**
     * @brief Retrieves the current event listener.
     *
     * @return Pointer to the current WindowEventListener.
     */
    WindowEventListener* getEventListener() {
        return mEventListener;
    }

    /**
     * @brief Resets the current buffer being used.
     *
     * This method marks the buffer as null to indicate it has been reset.
     */
    virtual void resetBuffer() {
        mBufferItem = nullptr;
    }

    /**
     * @brief Updates the visibility state of the UI.
     *
     * @param visible True to set the UI as visible, false to hide it.
     */
    virtual void updateVisibility(bool visible);

    /**
     * @brief Checks if Vsync events are enabled.
     *
     * @return True if Vsync events are enabled, false otherwise.
     */
    bool vsyncEventEnabled() {
        return mVsyncEnabled;
    }

    /**
     * @brief Requests a Vsync event.
     *
     * @param enable True to enable Vsync request, false to disable.
     */
    void onFBVsyncRequest(bool enable);

    /**
     * @brief Notifies the driver of a Vsync event.
     *
     * This method may be overridden to handle Vsync events.
     */
    virtual void notifyVsyncEvent() {}

    /**
     * @brief Retrieves the timer period for UI updates.
     *
     * @return Timer period in milliseconds.
     */
    virtual uint32_t getTimerPeriod() {
        return 16; // Default to 60 FPS (16ms per frame)
    }

    virtual bool needPeriodicVsync() {
        return false;
    }

    /**
     * @brief Traces the frame if enabled.
     *
     * @param enable True to enable frame tracing, false to disable.
     */
    void traceFrame(bool enable) {
        mTraceFrame = enable;
    }

    /**
     * @brief Retrieves frame metadata information if tracing is enabled.
     *
     * @return Pointer to FrameMetaInfo if tracing is enabled, otherwise nullptr.
     */
    FrameMetaInfo* frameMetaInfo() {
        return mTraceFrame ? &mFrameMetaInfo : nullptr;
    }

private:
    std::weak_ptr<BaseWindowDefault> mBaseWindow;
    BufferItem* mBufferItem;
    Rect mRectCrop;
    int8_t mFlags;
    InputMonitor* mInputMonitor;
    WindowEventListener* mEventListener;
    bool mVsyncEnabled;

    bool mTraceFrame;
    FrameMetaInfo mFrameMetaInfo;
};

} // namespace wm
} // namespace os
