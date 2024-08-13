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

#include <android-base/macros.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <binder/Status.h>
#include <utils/RefBase.h>

namespace os {
namespace wm {

using android::IBinder;
using android::Parcel;
using android::Parcelable;
using android::sp;
using android::status_t;

template <typename T>
class FakeFmq {
public:
    FakeFmq();
    ~FakeFmq();

    bool read(T* data) {
        if (!mQueue || mCaps <= 0 || !data) {
            return false;
        }

        auto current = mQueue[mReadPos];

        /* no valid item */
        if (current == 0) {
            return false;
        }

        *data = current;
        mReadPos = (mReadPos + 1) % mCaps;
        return true;
    }

    bool write(const T* data) {
        if (!mQueue || mCaps <= 0 || !data) {
            return false;
        }

        /*firstly write ending flag */
        auto next = (mWritePos + 1) % mCaps;
        mQueue[next] = 0;

        mQueue[mWritePos] = *data;
        mWritePos = next;
        return true;
    }

    bool create(const std::vector<T>& qData, bool isServer);
    void destroy();

    void setName(const std::string& name) {
        mName = name;
    }
    std::string getName() {
        return mName;
    }

    status_t writeToParcel(Parcel* out) const;
    status_t readFromParcel(const Parcel* in);
    void copyFrom(FakeFmq<T>& other);

private:
    std::string mName;
    int mFd;
    uint32_t mCaps;
    uint32_t mReadPos;
    uint32_t mWritePos;
    uint32_t mReserved;
    T* mQueue;
};

} // namespace wm
} // namespace os
