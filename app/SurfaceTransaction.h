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

#include <binder/IBinder.h>

#include <unordered_map>

#include "WindowManager.h"
#include "wm/BufferQueue.h"

namespace os {
namespace wm {

/**
 * @brief Forward declaration of LayerState, Rect, and SurfaceControl classes.
 */
class LayerState;
class Rect;
class SurfaceControl;
class WindowManager;

using android::IBinder;
using android::sp;

/**
 * @struct IBinderHash
 * @brief Hasher for IBinder shared pointers.
 *
 * This structure is used to define a hash function for
 * sp<IBinder> pointers, allowing them to be used as keys
 * in unordered maps.
 */
struct IBinderHash {
    std::size_t operator()(const sp<IBinder>& iBinder) const {
        return std::hash<IBinder*>{}(iBinder.get());
    }
};

/**
 * @class SurfaceTransaction
 * @brief Represents a transaction for surface operations.
 *
 * This class is responsible for managing and applying surface
 * transactions such as setting buffers, positions, and other
 * properties for surfaces in the window manager.
 */
class SurfaceTransaction {
public:
    SurfaceTransaction();

    ~SurfaceTransaction();

    /**
     * @brief Sets the buffer for a surface control.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @param item Reference to a BufferItem to be set.
     * @param seq Sequence number for the operation.
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& setBuffer(const std::shared_ptr<SurfaceControl>& sc, BufferItem& item,
                                  uint32_t seq);

    /**
     * @brief Sets the crop rectangle for a surface control.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @param rect Reference to a Rect object defining the crop area.
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& setBufferCrop(const std::shared_ptr<SurfaceControl>& sc, Rect& rect);

    /**
     * @brief Sets the position of a surface control.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @param x New X coordinate for the surface.
     * @param y New Y coordinate for the surface.
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& setPosition(const std::shared_ptr<SurfaceControl>& sc, int32_t x,
                                    int32_t y);

    /**
     * @brief Sets the alpha value of a surface control.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @param alpha New alpha value for the surface transparency.
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& setAlpha(const std::shared_ptr<SurfaceControl>& sc, int32_t alpha);

    /**
     * @brief Sets the sequence number for a surface control.
     *
     * @param sc Shared pointer to the SurfaceControl.
     * @param seq New sequence number.
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& setSequence(const std::shared_ptr<SurfaceControl>& sc, uint32_t seq);

    /**
     * @brief Applies the surface transaction.
     *
     * This method commits all the changes made to the surface transaction.
     *
     * @return Reference to the current SurfaceTransaction for method chaining.
     */
    SurfaceTransaction& apply();

    /**
     * @brief Sets the associated WindowManager.
     *
     * @param wm Pointer to the WindowManager.
     */
    void setWindowManager(WindowManager* wm) {
        mWindowManager = wm;
    }

    /**
     * @brief Cleans up the transaction state.
     *
     * This method resets the state of the transaction, preparing it for reuse.
     */
    void clean();

private:
    LayerState* getLayerState(const std::shared_ptr<SurfaceControl>& sc);

    std::unordered_map<sp<IBinder>, LayerState, IBinderHash> mLayerStates;
    WindowManager* mWindowManager;
};

} // namespace wm
} // namespace os
