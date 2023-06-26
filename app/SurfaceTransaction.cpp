
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

#include "SurfaceTransaction.h"

#include "wm/BufferQueue.h"
#include "wm/LayerState.h"
#include "wm/Rect.h"
#include "wm/SurfaceControl.h"

namespace os {
namespace wm {

SurfaceTransaction::SurfaceTransaction() {}

SurfaceTransaction::~SurfaceTransaction() {
    clean();
}

SurfaceTransaction& SurfaceTransaction::setBuffer(const std::shared_ptr<SurfaceControl>& sc,
                                                  BufferItem& item) {
    LayerState* state = getLayerState(sc);

    if (state != nullptr) {
        state->mFlags |= LayerState::LAYER_BUFFER_CHANGED;
        state->mBufferKey = item.mKey;
    }
    return *this;
}

SurfaceTransaction& SurfaceTransaction::setBufferCrop(const std::shared_ptr<SurfaceControl>& sc,
                                                      Rect& rect) {
    LayerState* state = getLayerState(sc);

    // TODO:
    if (state != nullptr) {
    }
    return *this;
}

SurfaceTransaction& SurfaceTransaction::setPosition(const std::shared_ptr<SurfaceControl>& sc,
                                                    int32_t x, int32_t y) {
    LayerState* state = getLayerState(sc);

    // TODO:
    if (state != nullptr) {
    }
    return *this;
}

SurfaceTransaction& SurfaceTransaction::setAlpha(const std::shared_ptr<SurfaceControl>& sc,
                                                 int alpha) {
    LayerState* state = getLayerState(sc);

    // TODO:
    if (state != nullptr) {
    }

    return *this;
}

SurfaceTransaction& SurfaceTransaction::apply() {
    // TODO:
    return *this;
}

void SurfaceTransaction::clean() {
    mLayerStates.clear();
}

LayerState* SurfaceTransaction::getLayerState(const std::shared_ptr<SurfaceControl>& sc) {
    sp<IBinder> key = sc->getToken();
    if (key == nullptr) {
        return nullptr;
    }

    if (mLayerStates.find(key) == mLayerStates.end()) {
        LayerState s(key);
        mLayerStates[key] = s;
    }

    return &mLayerStates[key];
}

} // namespace wm
} // namespace os