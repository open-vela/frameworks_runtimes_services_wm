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
    FLOGI("%p create surface for handle %p \n", this, mHandle.get());
}

SurfaceControl::~SurfaceControl() {
    FLOGI("%p free surface for handle %p \n", this, mHandle.get());
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

    if (mBufferIds.size() > 0) {
        mFreeMsgSlot.writeToParcel(out);
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

    if (size > 0) {
        mFreeMsgSlot.readFromParcel(in);
    }
    return android::OK;
}

void SurfaceControl::initBufferIds(const std::vector<BufferId>& ids) {
    mBufferIds = ids;
}

bool SurfaceControl::isValid() {
    if (mHandle == nullptr || mToken == nullptr) {
        FLOGD("invalid handle or client");
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
    mFreeMsgSlot.copyFrom(other.mFreeMsgSlot);
}

bool SurfaceControl::initFMQ(bool isServer) {
    if (mBufferIds.size() > 0) {
        std::vector<BufferKey> bufKeys;
        for (const auto& id : mBufferIds) {
            bufKeys.push_back(id.mKey);
        }
        return mFreeMsgSlot.create(bufKeys, isServer);
    }
    return false;
}

void SurfaceControl::destroyFMQ() {
    mFreeMsgSlot.destroy();
}

static inline bool initSharedBuffer(std::string name, int* pfd, int32_t size) {
    int32_t flag = O_RDWR | O_CLOEXEC;

    if (size > 0) flag |= O_CREAT;

    *pfd = -1;
    const char* cname = name.c_str();
    int fd = shm_open(cname, flag, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        FLOGE("failed to open shared memory %s, %s", cname, strerror(errno));
        return false;
    }

    /* only for server */
    if (size > 0 && ftruncate(fd, size) == -1) {
        int result = shm_unlink(cname);
        FLOGE("failed to resize shared memory for %s, size=%" PRId32 ", unlink return %d", cname,
              size, result);
        close(fd);

        // for debug:print info when resized failed
        system("ps; free; ls -l /var/shm");
        return false;
    }

    FLOGI("init shared memory success for %s, size=%" PRId32 " ", cname, size);
    *pfd = fd;
    return true;
}

static inline void uninitSharedBuffer(int fd, std::string name) {
    if (fd > 0) {
        int result = shm_unlink(name.c_str());
        FLOGI("uninit shared memory for %s, result=%d", name.c_str(), result);
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
    if (isServer && !result && ids.size() < 2) {
        for (const auto& id : ids) {
            uninitSharedBuffer(id.mFd, id.mName);
            close(id.mFd);
        }
        ids.clear();
    }

    FLOGI("%p", sc.get());

    /* update buffer ids*/
    sc->initBufferIds(ids);
    sc->initFMQ(isServer);
}

void uninitSurfaceBuffer(const std::shared_ptr<SurfaceControl>& sc) {
    if (sc.get() == nullptr) return;

    FLOGI("try to uninit shared memory");
    auto bufferIds = sc->bufferIds();
    for (auto it : bufferIds) {
        uninitSharedBuffer(it.mFd, it.mName);
    }
    sc->clearBufferIds();
    sc->destroyFMQ();
}

/**************** fmq ********************/
template <typename T>
FakeFmq<T>::FakeFmq() : mName(""), mFd(0), mCaps(0), mReadPos(0), mWritePos(0), mQueue(NULL) {}

template <typename T>
FakeFmq<T>::~FakeFmq() {
    destroy();
}

template <typename T>
status_t FakeFmq<T>::writeToParcel(Parcel* out) const {
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    SAFE_PARCEL(out->writeCString, mName.c_str());
#else
    SAFE_PARCEL(out->writeDupFileDescriptor, mFd);
#endif
    SAFE_PARCEL(out->writeUint32, mCaps);
    SAFE_PARCEL(out->writeUint32, mReadPos);
    SAFE_PARCEL(out->writeUint32, mWritePos);
    return android::OK;
}

template <typename T>
status_t FakeFmq<T>::readFromParcel(const Parcel* in) {
#ifdef CONFIG_ENABLE_BUFFER_QUEUE_BY_NAME
    mName = in->readCString();
    mFd = -1;
#else
    mName = "";
    mFd = dup(in->readFileDescriptor());
#endif
    SAFE_PARCEL(in->readUint32, &mCaps);
    SAFE_PARCEL(in->readUint32, &mReadPos);
    SAFE_PARCEL(in->readUint32, &mWritePos);
    return android::OK;
}

template <typename T>
void FakeFmq<T>::copyFrom(FakeFmq<T>& other) {
    mName = other.mName;
    mFd = 0;
    mCaps = other.mCaps;
    mReadPos = other.mReadPos;
    mWritePos = other.mWritePos;
    mQueue = NULL;
}

template <typename T>
void FakeFmq<T>::destroy() {
    if (!mQueue) {
        return;
    }

    FLOGI("now unmap and close shared memory for %d", mFd);
    uninitSharedBuffer(mFd, mName);
    if (mFd > 0 && close(mFd) == -1) {
        FLOGE("failed to close shared memory for %d", mFd);
    }

    mQueue = NULL;
    mFd = 0;
    mCaps = 0;
    mName = "";
    mReadPos = 0;
    mWritePos = 0;
}

template <typename T>
bool FakeFmq<T>::create(const std::vector<T>& qData, bool isServer) {
    auto bufCount = qData.size();

    if (bufCount <= 0 && mQueue == NULL) {
        FLOGW("cannot init empty fmq for %s", mName.c_str());
        return false;
    }

    /* free previous dirty queue */
    destroy();

    mCaps = qData.size() + 1;
    auto size = mCaps * sizeof(T);
    int fd = 0;
    if (!initSharedBuffer(mName, &fd, isServer ? size : 0)) {
        FLOGE("failed to init fmq for %s", mName.c_str());
        mCaps = 0;
        return false;
    }

    /* mmap */
    void* buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        FLOGE("failed to map fmq for %s", mName.c_str());
        uninitSharedBuffer(fd, mName);
        mCaps = 0;
        return false;
    }

    FLOGI("init fmq for %s", mName.c_str());

    memset(buffer, 0, size);
    mQueue = (T*)buffer;
    mFd = fd;

    int i = 0;
    for (const auto& value : qData) {
        mQueue[i++] = value;
    }

    mReadPos = 0;
    mWritePos = mCaps - 1;
    return true;
}

} // namespace wm
} // namespace os
