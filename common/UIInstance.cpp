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

#define LOG_TAG "UIInstance"

#include <lvgl/lvgl.h>

#if LVGL_VERSION_MAJOR >= 9

#include <nuttx/tls.h>

#include "LogUtils.h"
#include "lv_porting/lv_tick_interface.h"

#if LV_ENABLE_GLOBAL_CUSTOM == 0
#error "WMS depends on LV_ENABLE_GLOBAL_CUSTOM, please enable it by menuconfig"
#endif

#if CONFIG_TLS_TASK_NELEM == 0
#error "WMS depends on CONFIG_TLS_TASK_NELEM, please enable it by menuconfig"
#endif

void UIInit(void) {
    lv_init();
    lv_tick_set_cb(millis);
}

void UIDeinit(void) {
    lv_deinit();
}

static void lv_global_free(FAR void *data) {
    FLOGI("free ui instance for thread(%d)", pthread_self());
    if (data) {
        free(data);
    }
}

lv_global_t *lv_global_default(void) {
    static int index = -1;
    lv_global_t *data;

    if (index < 0) {
        index = task_tls_alloc(lv_global_free);
    }

    if (index >= 0) {
        data = (lv_global_t *)task_tls_get_value(index);
        if (data == NULL) {
            data = (lv_global_t *)calloc(1, sizeof(lv_global_t));
            task_tls_set_value(index, (uintptr_t)data);
            FLOGI("new ui instance for thread(%d)", pthread_self());
        }
    }
    return data;
}

#else

void UIInit(void) {
    lv_init();
}

void UIDeinit(void) {}

#endif