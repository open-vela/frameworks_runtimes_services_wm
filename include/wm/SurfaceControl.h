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
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

#include <unordered_map>

#include "wm/BufferQueue.h"

namespace os {
namespace wm {

using android::IBinder;
using android::Parcel;
using android::Parcelable;
using android::sp;
using android::status_t;

class SurfaceControl : public Parcelable {
public:
    SurfaceControl();
    ~SurfaceControl();

    SurfaceControl(const sp<IBinder>& token, const sp<IBinder>& handle, uint32_t width = 0,
                   uint32_t height = 0, uint8_t format = 0);

    status_t writeToParcel(Parcel* out) const override;
    status_t readFromParcel(const Parcel* in) override;

    bool isValid();

    std::unordered_map<BufferKey, BufferId>& bufferIds() {
        return mBufferIds;
    }

    void initBufferIds(std::vector<BufferId>& ids);

    void setBufferQueue(const std::shared_ptr<BufferQueue>& bq) {
        mBufferQueue = bq;
    }

    std::shared_ptr<BufferQueue> bufferQueue() {
        return mBufferQueue;
    }

    sp<IBinder> getToken() {
        return mToken;
    }

    sp<IBinder> getHandle() {
        return mHandle;
    }

    uint32_t getWidth() {
        return mWidth;
    }

    uint32_t getHeight() {
        return mHeight;
    }

    int32_t getFormat() {
        return mFormat;
    }

    static bool isSameSurface(const std::shared_ptr<SurfaceControl>& lhs,
                              const std::shared_ptr<SurfaceControl>& rhs);

private:
    // IWindow as token
    sp<IBinder> mToken;

    // SurfaceControl unique id
    sp<IBinder> mHandle;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;

    // key: server/client unique id
    std::unordered_map<BufferKey, BufferId> mBufferIds;
    std::shared_ptr<BufferQueue> mBufferQueue;
};

} // namespace wm
} // namespace os
