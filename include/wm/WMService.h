/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#include <binder/IServiceManager.h>
#include <os/wm/IWindowManager.h>
#include <utils/String8.h>

#include "app/UvLoop.h"

/**
 * @brief Starts the Window Manager (WM) service.
 *
 * This function initializes and starts the window manager service
 * with the provided service manager and UV loop instance. It sets
 * up the necessary components for window management and event handling.
 *
 * @param sm A shared pointer to the IServiceManager used for managing
 *           system services.
 * @param uvLooper A shared pointer to the UvLoop instance used for
 *                 asynchronous event handling.
 * @return A shared pointer to the IWindowManager interface for
 *         interacting with the window manager service.
 */
::android::sp<::os::wm::IWindowManager> startWMService(::android::sp<::android::IServiceManager> sm,
                                                       std::shared_ptr<::os::app::UvLoop> uvLooper);
