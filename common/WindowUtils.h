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

#include <inttypes.h>
#include <nuttx/config.h>
#include <time.h>
#include <utils/Log.h>

#include "ParcelUtils.h"
#include "WindowTrace.h"

#ifdef CONFIG_WINDOW_SERVICE_LOG_LEVEL
#define WM_LOG_LEVEL CONFIG_WINDOW_SERVICE_LOG_LEVEL
#else
#define WM_LOG_LEVEL 4 // default warning
#endif

#ifdef CONFIG_ALOG
#define print_wm_log(level, fmt, ...)                             \
    {                                                             \
        if (level <= WM_LOG_LEVEL) {                              \
            ALOG(level, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
        }                                                         \
    }

#define FLOGE(fmt, ...) print_wm_log(LOG_ERROR, fmt, ##__VA_ARGS__)
#define FLOGW(fmt, ...) print_wm_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define FLOGI(fmt, ...) print_wm_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define FLOGD(fmt, ...) print_wm_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define FLOGV(fmt, ...) print_wm_log(LOG_VERBOSE, fmt, ##__VA_ARGS__)
#else
#include <syslog.h>

#define print_wm_log(level, fmt, ...)                                             \
    {                                                                             \
        if (level <= WM_LOG_LEVEL) {                                              \
            syslog(level, "[" LOG_TAG "] %s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
        }                                                                         \
    }

#define FLOGE(fmt, ...) print_wm_log(LOG_ERR, fmt, ##__VA_ARGS__)
#define FLOGW(fmt, ...) print_wm_log(LOG_WARNING, fmt, ##__VA_ARGS__)
#define FLOGI(fmt, ...) print_wm_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define FLOGD(fmt, ...) print_wm_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define FLOGV(fmt, ...) print_wm_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#endif

uint32_t getLvColorFormatType(uint32_t format);
uint64_t curSysTimeMs(void);
uint64_t curSysTimeUs(void);
uint64_t curSysTimeNs(void);

#define DATA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define DATA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DATA_CLAMP(val, min, max) (DATA_MAX(min, (DATA_MIN(val, max))))
