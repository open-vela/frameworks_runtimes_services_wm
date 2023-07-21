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

#include "UIDriverProxy.h"

namespace os {
namespace wm {
using DUMMY_DRAW_CALLBACK = std::function<void(void*, uint32_t)>;

class DummyDriverProxy : public UIDriverProxy {
public:
    DummyDriverProxy(std::shared_ptr<BaseWindow> win);
    ~DummyDriverProxy();

    void* getRoot() override;
    void* getWindow() override;
    bool initUIInstance() override;
    void handleEvent(InputMessage& message) override;
    void drawFrame(BufferItem* bufItem) override;

    void setDrawCallback(const DUMMY_DRAW_CALLBACK& cb) {
        mDrawCallback = cb;
    }

private:
    DUMMY_DRAW_CALLBACK mDrawCallback;
};

} // namespace wm
} // namespace os
