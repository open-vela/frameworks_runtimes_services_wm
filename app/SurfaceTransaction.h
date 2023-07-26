
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

class LayerState;
class Rect;
class SurfaceControl;
class WindowManager;

using android::IBinder;
using android::sp;

struct IBinderHash {
    std::size_t operator()(const sp<IBinder>& iBinder) const {
        return std::hash<IBinder*>{}(iBinder.get());
    }
};

class SurfaceTransaction {
public:
    SurfaceTransaction();
    ~SurfaceTransaction();

    SurfaceTransaction& setBuffer(const std::shared_ptr<SurfaceControl>& sc, BufferItem& item);
    SurfaceTransaction& setBufferCrop(const std::shared_ptr<SurfaceControl>& sc, Rect& rect);

    SurfaceTransaction& setPosition(const std::shared_ptr<SurfaceControl>& sc, int32_t x,
                                    int32_t y);

    SurfaceTransaction& setAlpha(const std::shared_ptr<SurfaceControl>& sc, int alpha);

    SurfaceTransaction& apply();

    void setWindowManager(WindowManager* wm) {
        mWindowManager = wm;
    }

    void clean();

private:
    LayerState* getLayerState(const std::shared_ptr<SurfaceControl>& sc);

    std::unordered_map<sp<IBinder>, LayerState, IBinderHash> mLayerStates;
    WindowManager* mWindowManager;
};

} // namespace wm
} // namespace os
