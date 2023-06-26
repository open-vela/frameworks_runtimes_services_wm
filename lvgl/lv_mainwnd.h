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

/**
 * @file lv_mainwnd.h
 *
 */

#ifndef LV_MAINWND_H
#define LV_MAINWND_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <lvgl/lvgl.h>

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    int id;
    lv_coord_t w;
    lv_coord_t h;
    uint32_t cf : 5;
    uint32_t data_size;
    void* data;
} lv_mainwnd_buf_dsc_t;

typedef struct {
    // 1: for key; 2: for pointer
    int type : 8;
    union {
        struct {
            uint32_t key;
        } keypad;
        struct {
            uint32_t x;
            uint32_t y;
        } pointer;
    };
    // 0: for release; 1: for pressed
    int state : 8;
} lv_mainwnd_input_event_t;

typedef struct _lv_mainwnd_metainfo_t {
    // acquire buffer before drawing content
    bool (*acquire_buffer)(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc);

    // release buffer after drawing content
    bool (*release_buffer)(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc);

    // collect input data and send it to observer
    int (*send_input_event)(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_input_event_t* event);

    // notify caller state (destroy)
    bool (*on_destroy)(struct _lv_mainwnd_metainfo_t* meta, lv_mainwnd_buf_dsc_t* buf_dsc);
    void* info;
} lv_mainwnd_metainfo_t;

typedef struct {
    lv_obj_t obj;
    lv_mainwnd_metainfo_t meta_info;
    lv_mainwnd_buf_dsc_t buf_dsc;
} lv_mainwnd_t;

extern const lv_obj_class_t lv_mainwnd_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an main window object
 * @param parent        pointer to an object, it will be the parent of the new main window
 * @return pointer to the created main window
 */
lv_obj_t* lv_mainwnd_create(lv_obj_t* parent);

/**
 * Update buffer.
 * @param obj           pointer to a main window object
 * @param buf_dsc       pointer to the buffer descriptor
 * @param area          pointer to an area to update
 */
void lv_mainwnd_update_buffer(lv_obj_t* obj, lv_mainwnd_buf_dsc_t* buf_dsc, const lv_area_t* area);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the meta info to init the lv_mainwnd_metainfo_t.
 * @param obj           pointer to a main window object
 * @param metainfo      pointer to the meta info
 */
void lv_mainwnd_set_metainfo(lv_obj_t* obj, lv_mainwnd_metainfo_t* metainfo);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_MAINWND_H*/