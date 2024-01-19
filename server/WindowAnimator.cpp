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

#define LOG_TAG "WindowAnimator"

#include "WindowAnimator.h"

#include <string>

#include "../common/WindowUtils.h"

#ifdef CONFIG_ENABLE_TRANSITION_ANIMATION
namespace os {
namespace wm {
#ifdef CONFIG_ANIMATION_ENGINE
static inline void on_anim_status_cb(anim_layer_t* layer, const anim_status_type_t status) {
    if (status == ANIM_ST_END) {
        FLOGW("status  : layer obj:%p, user data:%p, layer type: %d, property type: %d, status: "
              "%d  \n",
              layer->layer_object, layer->user_data, layer->layer_type, layer->property_type,
              (int)status);
        WindowAnimator* winAnim = reinterpret_cast<WindowAnimator*>(layer->user_data);
        winAnim->mAnimStatus = WINDOW_ANIM_STATUS_FINISHED;
        winAnim->mAnimStatusCB(winAnim->mAnimStatus);
    }
}
#endif

WindowAnimator::WindowAnimator(AnimEngineHandle animEngine, lv_obj_t* widget)
      : mAnimEngine(animEngine), mWidget(widget) {
    mAnimStatus = WINDOW_ANIM_STATUS_FINISHED;
}

WindowAnimator::~WindowAnimator() {
    if (mAnimStatus != WINDOW_ANIM_STATUS_FINISHED) {
#ifdef CONFIG_ANIMATION_ENGINE
        anim_listener(mAnimEngine, mAnimId, nullptr, nullptr, (void*)this);
#endif
        cancelAnimation();
    }
}

int WindowAnimator::startAnimation(std::string animConfig, AnimCallback callback) {
    FLOGI("%p", this);
    mAnimStatus = WINDOW_ANIM_STATUS_STARTING;
    int ret = -1;
#ifdef CONFIG_ANIMATION_ENGINE
    ret = anim_create(mAnimEngine, &mAnimId, animConfig.c_str());
    mAnimStatusCB = callback;
    ret = anim_start(mAnimEngine, mAnimId, mWidget, anim_layer_type_t::ANIM_LT_NORMAL);
    anim_listener(mAnimEngine, mAnimId, on_anim_status_cb, nullptr, (void*)this);
#endif
    return ret;
}

void WindowAnimator::cancelAnimation() {
    FLOGI("%p", this);
#ifdef CONFIG_ANIMATION_ENGINE
    anim_remove(mAnimEngine, mAnimId);
#endif
}

} // namespace wm
} // namespace os

#endif
