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
 * @file lv_mainwnd.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_mainwnd.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_mainwnd_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_mainwnd_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void lv_mainwnd_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void lv_mainwnd_event(const lv_obj_class_t* class_p, lv_event_t* e);
static void lv_draw_mainwnd(lv_event_t* e);
static void lv_mainwnd_buf_dsc_reset(lv_obj_t* obj);
static void lv_mainwnd_metainfo_reset(lv_obj_t* obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_mainwnd_class = {
    .constructor_cb = lv_mainwnd_constructor,
    .destructor_cb = lv_mainwnd_destructor,
    .event_cb = lv_mainwnd_event,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .instance_size = sizeof(lv_mainwnd_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_mainwnd_create(lv_obj_t* parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t* obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_mainwnd_update_buffer(lv_obj_t* obj, lv_mainwnd_buf_dsc_t* buf_dsc, const lv_area_t* area)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    if (!buf_dsc) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        lv_mainwnd_buf_dsc_reset(obj);
        return;
    }

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    mainwnd->buf_dsc.id = buf_dsc->id;
    mainwnd->buf_dsc.data = buf_dsc->data;
    mainwnd->buf_dsc.data_size = buf_dsc->data_size;
    mainwnd->buf_dsc.cf = buf_dsc->cf;
    mainwnd->buf_dsc.w = buf_dsc->w;
    mainwnd->buf_dsc.h = buf_dsc->h;

    if (!area) {
        lv_obj_invalidate(obj);
        return;
    }
    lv_area_t win_coords;
    lv_obj_get_coords(obj, &win_coords);

    lv_area_t common_area;
    if (!_lv_area_intersect(&common_area, &win_coords, area))
        return;
    lv_obj_invalidate_area(obj, &common_area);
}

/*=====================
 * Setter functions
 *====================*/

void lv_mainwnd_set_metainfo(lv_obj_t* obj, lv_mainwnd_metainfo_t* metainfo)
{
    if (!metainfo) {
        LV_LOG_WARN("lv_mainwnd_set_metainfo: metainfo is NULL");
        lv_mainwnd_metainfo_reset(obj);
        return;
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    mainwnd->meta_info = *metainfo;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_mainwnd_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_mainwnd_metainfo_reset(obj);

    lv_mainwnd_buf_dsc_reset(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_mainwnd_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
    LV_UNUSED(class_p);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    if (mainwnd->meta_info.on_destroy) {
        mainwnd->meta_info.on_destroy(&mainwnd->meta_info, &mainwnd->buf_dsc);
    }
}

static void lv_mainwnd_event(const lv_obj_class_t* class_p, lv_event_t* e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK)
        return;

    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t* obj = lv_event_get_target(e);
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    if (code == LV_EVENT_DRAW_MAIN) {
        if (mainwnd->meta_info.acquire_buffer && mainwnd->buf_dsc.id == -1) {
            if (!mainwnd->meta_info.acquire_buffer(&mainwnd->meta_info, &mainwnd->buf_dsc)) {
                LV_LOG_WARN("lv_mainwnd acquire_buffer FAILED");
                return;
            }
            lv_draw_mainwnd(e);
        }
    }
}

static void lv_draw_mainwnd(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK)
        return;

    if (mainwnd->buf_dsc.data == NULL) {
        LV_LOG_WARN("lv_draw_mainwnd: buffer source is NULL");
        return;
    }

    if (mainwnd->buf_dsc.h == 0 || mainwnd->buf_dsc.w == 0) {
        LV_LOG_WARN("lv_draw_mainwnd: buffer size is 0");
        return;
    }

    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);

    lv_area_t win_coords;
    lv_area_copy(&win_coords, &obj->coords);

    win_coords.y1 = win_coords.y1;
    win_coords.y2 = win_coords.y1 + mainwnd->buf_dsc.h - 1;
    win_coords.x1 = win_coords.x1;
    win_coords.x2 = win_coords.x1 + mainwnd->buf_dsc.w - 1;

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_obj_init_draw_img_dsc(obj, LV_PART_MAIN, &img_dsc);

    img_dsc.zoom = LV_IMG_ZOOM_NONE;
    img_dsc.angle = 0;
    img_dsc.pivot.x = mainwnd->buf_dsc.w / 2;
    img_dsc.pivot.y = mainwnd->buf_dsc.h / 2;
    img_dsc.antialias = 0;

    lv_draw_img(draw_ctx, &img_dsc, &win_coords, mainwnd->buf_dsc.data);
}

static void lv_mainwnd_buf_dsc_reset(lv_obj_t* obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    LV_LOG_INFO("Reset mainwnd buffer descriptor");
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    mainwnd->buf_dsc.id = -1;
    mainwnd->buf_dsc.data = NULL;
    mainwnd->buf_dsc.data_size = 0;
    mainwnd->buf_dsc.cf = LV_IMG_CF_UNKNOWN;
    mainwnd->buf_dsc.w = lv_obj_get_width(obj);
    mainwnd->buf_dsc.h = lv_obj_get_height(obj);
}

static void lv_mainwnd_metainfo_reset(lv_obj_t* obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    LV_LOG_INFO("Reset mainwnd metainfo");
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    mainwnd->meta_info.acquire_buffer = NULL;
    mainwnd->meta_info.release_buffer = NULL;
    mainwnd->meta_info.send_input_event = NULL;
    mainwnd->meta_info.on_destroy = NULL;
    mainwnd->meta_info.info = NULL;
}