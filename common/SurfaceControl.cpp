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

#define LOG_TAG "SurfaceControl"

#include "wm/SurfaceControl.h"

#include <sys/mman.h>
#include <sys/stat.h>

#include "WindowUtils.h"

namespace os {
namespace wm {

bool SurfaceControl::isSameSurface(const std::shared_ptr<SurfaceControl>& lhs,
                                   const std::shared_ptr<SurfaceControl>& rhs) {
    if (lhs == nullptr || rhs == nullptr) return false;

    return lhs->mHandle == rhs->mHandle;
}

SurfaceControl::SurfaceControl() {}

SurfaceControl::SurfaceControl(const sp<IBinder>& token, const sp<IBinder>& handle, uint32_t width,
                               uint32_t height, uint32_t format, uint32_t size)
      : mToken(token),
        mHandle(handle),
        mWidth(width),
        mHeight(height),
        mFormat(format),
        mBufferSize(size) {
    FLOGI("%p, create surface for handle %p \n", this, mHandle.get());
}

SurfaceControl::~SurfaceControl() {
    FLOGI("%p, free surface for handle %p \n", this, mHandle.get());
    clearBufferIds();
    mToken.clear();
    mHandle.clear();
    mBufferQueue = nullptr;
}

status_t SurfaceControl::writeToParcel(Parcel* out) const {
    SAFE_PARCEL(out->writeStrongBinder, mToken);
    SAFE_PARCEL(out->writeStrongBinder, mHandle);
    SAFE_PARCEL(out->writeUint32, mWidth);
    SAFE_PARCEL(out->writeUint32, mHeight);
    SAFE_PARCEL(out->writeUint32, mFormat);
    SAFE_PARCEL(out->writeUint32, mBufferSize);

    SAFE_PARCEL(out->writeInt32, mBufferIds.size());
    for (const auto& id : mBufferIds) {
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        SAFE_PARCEL(out->writeCString, id.mName.c_str());
#else
        SAFE_PARCEL(out->writeDupFileDescriptor, id.mFd);
#endif
        SAFE_PARCEL(out->writeInt32, id.mKey);
    }
    return android::OK;
}

status_t SurfaceControl::readFromParcel(const Parcel* in) {
    mToken = in->readStrongBinder();
    mHandle = in->readStrongBinder();
    SAFE_PARCEL(in->readUint32, &mWidth);
    SAFE_PARCEL(in->readUint32, &mHeight);
    SAFE_PARCEL(in->readUint32, &mFormat);
    SAFE_PARCEL(in->readUint32, &mBufferSize);

    int32_t size;
    SAFE_PARCEL(in->readInt32, &size);
    for (int32_t i = 0; i < size; i++) {
        BufferId buffId;
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
        buffId.mName = in->readCString();
        buffId.mFd = -1;
#else
        buffId.mName = "";
        buffId.mFd = dup(in->readFileDescriptor());
#endif
        SAFE_PARCEL(in->readInt32, &buffId.mKey);
        mBufferIds.push_back(buffId);
    }
    return android::OK;
}

void SurfaceControl::initBufferIds(const std::vector<BufferId>& ids) {
    mBufferIds = ids;
}

bool SurfaceControl::isValid() {
    if (mHandle == nullptr || mToken == nullptr) {
        FLOGD("invalid handle (%p) or client (%p)", mHandle.get(), mToken.get());
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
    mBufferSize = other.mBufferSize;
    mBufferIds = other.mBufferIds;
}

static inline bool initSharedBuffer(std::string name, int* pfd, int32_t size) {
    int32_t flag = O_RDWR | O_CLOEXEC;

    if (size > 0) flag |= O_CREAT;

    *pfd = -1;
    const char* cname = name.c_str();
    int fd = shm_open(cname, flag, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        FLOGE("failed to open shared memory for %s, %s", cname, strerror(errno));
        return false;
    }

    /* only for server */
    if (size > 0 && ftruncate(fd, size) == -1) {
        int result = shm_unlink(cname);
        FLOGE("failed to resize shared memory for %s, unlink return %d", cname, result);
        close(fd);
        return false;
    }

    FLOGI("init shared memory success for %s, size is %" PRId32 " ", cname, size);
    *pfd = fd;
    return true;
}

static inline void uninitSharedBuffer(int fd, std::string name) {
    if (fd > 0) {
        int result = shm_unlink(name.c_str());
        FLOGI("uninit shared memory %s, result: %d", name.c_str(), result);
    }
}

/* Surface control only record bufferIds, don't control shared memory's lifecycle */

void initSurfaceBuffer(const std::shared_ptr<SurfaceControl>& sc, bool isServer) {
    if (sc.get() == nullptr) return;

    auto bufferIds = sc->bufferIds();
    std::vector<BufferId> ids;
    bool result = true;

    int32_t size = isServer ? sc->getBufferSize() : 0;

    /* create shared memory */
    for (const auto& id : bufferIds) {
        int fd = -1;

        if (!initSharedBuffer(id.mName, &fd, size)) {
            result = false;
            break;
        }
        ids.push_back({id.mName, id.mKey, fd});
    }

    /* failure : free shared memory*/
    if (isServer && !result) {
        for (const auto& id : ids) {
            uninitSharedBuffer(id.mFd, id.mName);
            close(id.mFd);
        }
        ids.clear();
    }

    FLOGI("%p", sc.get());
    /* update buffer ids*/
    sc->initBufferIds(ids);
}

void uninitSurfaceBuffer(const std::shared_ptr<SurfaceControl>& sc) {
    if (sc.get() == nullptr) return;

    FLOGI("try to uninit shared memory");
    auto bufferIds = sc->bufferIds();
    for (auto it : bufferIds) {
        uninitSharedBuffer(it.mFd, it.mName);
    }
    sc->clearBufferIds();
}

} // namespace wm
} // namespace os
