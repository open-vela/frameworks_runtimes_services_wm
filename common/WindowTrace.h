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

#include <nuttx/trace.h>

#define WM_PROFILER_BEGIN() sched_note_beginex(NOTE_TAG_GRAPHICS, __func__);
#define WM_PROFILER_END() sched_note_endex(NOTE_TAG_GRAPHICS, __func__);

#define WM_PROFILER_BEGINEX(str) sched_note_beginex(NOTE_TAG_GRAPHICS, str);
#define WM_PROFILER_ENDEX(str) sched_note_endex(NOTE_TAG_GRAPHICS, str);