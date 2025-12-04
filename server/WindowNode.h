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

#include "WindowState.h"
#include "lvgl/lv_mainwnd.h"
#include "wm/InputMessage.h"
#include "wm/Rect.h"

/**
 * @namespace os::wm
 * @brief The namespace for window management related classes and functionalities.
 */
namespace os {
namespace wm {

/**
 * @class WindowNode
 * @brief Represents a node in the window management tree.
 *
 * This class encapsulates the properties and behaviors of a window node
 * including its associated state, dimensions, and the ability to process
 * and manage rendering buffers. It serves as a building block for layering
 * windows in the graphical interface.
 */
class WindowNode {
public:
    WindowNode(WindowState* state, void* parent, const Rect& rect, bool enableInput,
               int32_t format);

    ~WindowNode();

    DISALLOW_COPY_AND_ASSIGN(WindowNode);

    /**
     * @brief Retrieves the rectangle defining this window node.
     *
     * @return Reference to the Rect representing the size and position.
     */
    Rect& getRect() {
        return mRect;
    }

    /**
     * @brief Retrieves the associated WindowState.
     *
     * @return Pointer to the associated WindowState object.
     */
    WindowState* getState() {
        return mState;
    }

    /**
     * @brief Retrieves the color format of this window node.
     *
     * @return The color format used for rendering.
     */
    lv_color_format_t getColorFormat() {
        return mColorFormat;
    }

    /**
     * @brief Retrieves the LVGL widget associated with this window node.
     *
     * @return Pointer to the LVGL object used for rendering.
     */
    lv_obj_t* getWidget() {
        return mWidget;
    }

    /**
     * @brief Updates the rectangle for this window node.
     *
     * @param newRect The new Rect to set for this window node.
     */
    void setRect(const Rect& newRect);

    /**
     * @brief Sets the parent of this window node in the hierarchy.
     *
     * @param parent A pointer to the new parent node.
     */
    void setParent(void* parent);

    /**
     * @brief Resets the opaque status of this window node.
     *
     * This method prepares the window node for rendering by resetting
     * its visual settings.
     */
    void resetOpaque();

    /**
     * @brief Retrieves the size of the surface associated with this window node.
     *
     * @return The size of the surface in pixels.
     */
    uint32_t getSurfaceSize();

    /**
     * @brief Updates the buffer for the WindowNode for multi-instance mode.
     *
     * This method updates the rendering buffer and the rectangle associated with
     * the node.
     *
     * @param item Pointer to the BufferItem that needs to be updated.
     * @param rect Pointer to the Rect defining the new position and size.
     * @param seq The sequence number for this operation.
     * @return True if the buffer update is successful, otherwise false.
     */
    bool updateBuffer(BufferItem* item, Rect* rect, uint32_t seq);

    /**
     * @brief Acquires a buffer from the buffer queue for multi-instance mode.
     *
     * This method retrieves a buffer that can be used for rendering
     * by this window node.
     *
     * @return Pointer to the acquired BufferItem.
     */
    BufferItem* acquireBuffer();

    /**
     * @brief Releases a buffer back to the queue for multi-instance mode.
     *
     * This method marks the given buffer as available for reuse.
     *
     * @return True if the buffer is released successfully, otherwise false.
     */
    bool releaseBuffer();

    /**
     * @brief Sets the visibility of this window node from client requesting.
     *
     * @param visibility The new visibility value to set.
     * */
    void setVisibility(int32_t visibility);

    /**
     * @brief Synchronizes the client visibility state with the window node.
     *
     * @param visibility The visibility state to synchronize, typically one of:
     *                   - LayoutParams::WINDOW_VISIBLE
     *                   - LayoutParams::WINDOW_INVISIBLE
     *                   - LayoutParams::WINDOW_GONE
     */
    void syncClientVisibility(int32_t visibility);

    int32_t getVisibility() {
        return mVisibility;
    }

    /**
     * @brief Retrieves the client screen associated with this window node for lite mode.
     *
     * @return Pointer to the client screen LVGL object.
     * */
    lv_obj_t* getClientScreen() {
        return mClientScreen;
    }

    /**
     * @brief Sends an input message to this window.
     *
     * @param ie Pointer to the InputMessage to send.
     * @return True if the message was successfully sent, false otherwise.
     */
    bool sendInputMessage(const InputMessage* ie);

private:
    WindowState* mState;
    BufferItem* mBuffer;
    lv_obj_t* mWidget;
    Rect mRect;
    lv_color_format_t mColorFormat;
    lv_obj_t* mClientScreen;
    int32_t mVisibility;
    InputMessage mLastInputMsg;
};

} // namespace wm
} // namespace os
