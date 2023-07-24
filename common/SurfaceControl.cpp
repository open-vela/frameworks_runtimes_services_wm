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

#include "wm/SurfaceControl.h"

#include "ParcelUtils.h"

namespace os {
namespace wm {
bool SurfaceControl::isSameSurface(const std::shared_ptr<SurfaceControl>& lhs,
                                   const std::shared_ptr<SurfaceControl>& rhs) {
    if (lhs == nullptr || rhs == nullptr) return false;

    return lhs->mHandle == rhs->mHandle;
}
SurfaceControl::SurfaceControl() {}

SurfaceControl::SurfaceControl(const sp<IBinder>& token, const sp<IBinder>& handle, uint32_t width,
                               uint32_t height, uint8_t format)
      : mToken(token), mHandle(handle), mWidth(width), mHeight(height), mFormat(format) {}

SurfaceControl::~SurfaceControl() {
    mToken.clear();
    mHandle.clear();
    // TODO:
}

status_t SurfaceControl::writeToParcel(Parcel* out) const {
    SAFE_PARCEL(out->writeStrongBinder, mToken);
    SAFE_PARCEL(out->writeStrongBinder, mHandle);
    SAFE_PARCEL(out->writeUint32, mWidth);
    SAFE_PARCEL(out->writeUint32, mHeight);
    SAFE_PARCEL(out->writeUint32, mFormat);

    SAFE_PARCEL(out->writeInt32, mBufferIds.size());
    for (const auto& entry : mBufferIds) {
        SAFE_PARCEL(out->writeInt32, entry.first);
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        SAFE_PARCEL(out->writeCString, entry.second.mName.c_str());
#else
        SAFE_PARCEL(out->writeDupFileDescriptor, entry.second.mFd);
#endif
        SAFE_PARCEL(out->writeInt32, entry.second.mKey);
    }
    return android::OK;
}

status_t SurfaceControl::readFromParcel(const Parcel* in) {
    mToken = in->readStrongBinder();
    mHandle = in->readStrongBinder();
    SAFE_PARCEL(in->readUint32, &mWidth);
    SAFE_PARCEL(in->readUint32, &mHeight);
    SAFE_PARCEL(in->readUint32, &mFormat);

    int32_t size;
    SAFE_PARCEL(in->readInt32, &size);
    for (int i = 0; i < size; i++) {
        BufferKey buffKey;
        SAFE_PARCEL(in->readInt32, &buffKey);

        BufferId buffId;
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        buffId.mName = in->readCString();
#else
        buffId.mFd = dup(in->readFileDescriptor());
#endif
        SAFE_PARCEL(in->readInt32, &buffId.mKey);
        mBufferIds[buffKey] = buffId;
    }
    return android::OK;
}

void SurfaceControl::initBufferIds(std::vector<BufferId>& ids) {
    mBufferIds.clear();
    for (uint32_t i = 0; i < ids.size(); i++) {
        mBufferIds.insert(std::make_pair(ids[i].mKey, ids[i]));
    }
}

bool SurfaceControl::isValid() {
    if (mHandle == nullptr || mToken == nullptr) {
        ALOGE("invalid handle (%p) or client (%p)", mHandle.get(), mToken.get());
        return false;
    }

    return !mBufferIds.empty();
}

void SurfaceControl::copyFrom(SurfaceControl& other) {
    mToken = other.mToken;
    mHandle = other.mHandle;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mFormat = other.mFormat;
    mBufferIds = other.mBufferIds;
}

} // namespace wm
} // namespace os
