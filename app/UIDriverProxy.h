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

#include "BaseWindow.h"
#include "wm/BufferQueue.h"
#include "wm/InputMessage.h"
#include "wm/Rect.h"

namespace os {
namespace wm {

class BaseWindow;

class UIDriverProxy {
public:
    UIDriverProxy(std::shared_ptr<BaseWindow> win);
    virtual ~UIDriverProxy();

    virtual void* getRoot() = 0;
    virtual void* getWindow() = 0;

    // window request ui proxy, update buffer data
    virtual void drawFrame(BufferItem* item);
    bool finishDrawing();

    // ui proxy response window
    bool onInvalidate(bool periodic);
    void* onDequeueBuffer();
    bool onQueueBuffer();
    void onCancelBuffer();

    void onRectCrop(Rect& rect);
    Rect* rectCrop();

    BufferItem* getBufferItem() {
        return mBufferItem;
    }

    virtual void handleEvent() = 0;
    bool readEvent(InputMessage* message);
    virtual bool enableInput(bool enable);

    virtual void updateResolution(int32_t width, int32_t height);

    enum {
        UIP_BUFFER_UPDATE = 1,
        UIP_BUFFER_RECT_UPDATE = 2,
    };
    void setEventListener(WindowEventListener* listener) {
        mEventListener = listener;
    }
    WindowEventListener* getEventListener() {
        return mEventListener;
    }

private:
    std::weak_ptr<BaseWindow> mBaseWindow;
    BufferItem* mBufferItem;
    Rect mRectCrop;
    int8_t mFlags;
    WindowEventListener* mEventListener;
};

} // namespace wm
} // namespace os
