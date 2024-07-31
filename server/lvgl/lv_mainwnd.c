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

#include "../common/WindowTrace.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_mainwnd_class
#define INVALID_BUFID -1

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_mainwnd_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void lv_mainwnd_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void lv_mainwnd_event(const lv_obj_class_t* class_p, lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_mainwnd_class = {.constructor_cb = lv_mainwnd_constructor,
                                         .destructor_cb = lv_mainwnd_destructor,
                                         .event_cb = lv_mainwnd_event,
                                         .width_def = LV_PCT(100),
                                         .height_def = LV_PCT(100),
                                         .instance_size = sizeof(lv_mainwnd_t),
                                         .base_class = &lv_obj_class};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline void reset_buf_dsc(lv_obj_t* obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    mainwnd->buf_dsc.id = INVALID_BUFID;
    mainwnd->buf_dsc.seq = 0;

    mainwnd->buf_dsc.img_dsc.data = NULL;
    mainwnd->buf_dsc.img_dsc.data_size = 0;
    mainwnd->buf_dsc.img_dsc.header.cf = LV_COLOR_FORMAT_UNKNOWN;
    mainwnd->buf_dsc.img_dsc.header.w = 0;
    mainwnd->buf_dsc.img_dsc.header.h = 0;
}

static inline void reset_meta_info(lv_obj_t* obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    mainwnd->meta_info.acquire_buffer = NULL;
    mainwnd->meta_info.release_buffer = NULL;
    mainwnd->meta_info.send_input_event = NULL;
    mainwnd->meta_info.on_destroy = NULL;
    mainwnd->meta_info.info = NULL;
}

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_mainwnd_create(lv_obj_t* parent) {
    LV_LOG_INFO("begin");
    lv_obj_t* obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

bool lv_mainwnd_update_buffer(lv_obj_t* obj, lv_mainwnd_buf_dsc_t* buf_dsc, const lv_area_t* area) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    WM_PROFILER_BEGIN();

    if (!buf_dsc) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        reset_buf_dsc(obj);
        WM_PROFILER_END();
        return true;
    }

    if (!buf_dsc->img_dsc.data || buf_dsc->img_dsc.header.w == 0 ||
        buf_dsc->img_dsc.header.h == 0) {
        WM_PROFILER_END();
        return false;
    }

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    if (mainwnd->buf_dsc.id < 0 && lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }

    mainwnd->buf_dsc.id = buf_dsc->id;
    mainwnd->buf_dsc.seq = buf_dsc->seq;
    mainwnd->buf_dsc.img_dsc.data = buf_dsc->img_dsc.data;
    mainwnd->buf_dsc.img_dsc.data_size = buf_dsc->img_dsc.data_size;
    mainwnd->buf_dsc.img_dsc.header.cf = buf_dsc->img_dsc.header.cf;
    mainwnd->buf_dsc.img_dsc.header.w = buf_dsc->img_dsc.header.w;
    mainwnd->buf_dsc.img_dsc.header.h = buf_dsc->img_dsc.header.h;

    if (!area) {
        lv_obj_invalidate(obj);
        WM_PROFILER_END();
        return true;
    }

    lv_area_t win_coords;
    lv_obj_get_coords(obj, &win_coords);

    if (win_coords.x1 != 0 || win_coords.y1 != 0)
        lv_area_move((lv_area_t*)area, win_coords.x1, win_coords.y1);

    lv_area_t inv_area;
    if (_lv_area_intersect(&inv_area, &win_coords, area)) {
        lv_obj_invalidate_area(obj, &inv_area);
    }
    WM_PROFILER_END();
    return true;
}

void lv_mainwnd_update_flag(lv_obj_t* obj, lv_mainwnd_flag_e flag, bool bAdd) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    WM_PROFILER_BEGIN();

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    if (bAdd) {
        mainwnd->flags |= flag;
    } else {
        mainwnd->flags &= ~flag;
    }
    WM_PROFILER_END();
}

/*=====================
 * Setter functions
 *====================*/

void lv_mainwnd_set_metainfo(lv_obj_t* obj, lv_mainwnd_metainfo_t* metainfo) {
    if (!metainfo) {
        reset_meta_info(obj);
        return;
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    mainwnd->meta_info = *metainfo;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_mainwnd_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    mainwnd->buf_dsc.id = INVALID_BUFID;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_mainwnd_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);

    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    if (mainwnd->buf_dsc.id == INVALID_BUFID) {
        return;
    }

    if (mainwnd->meta_info.release_buffer) {
        mainwnd->meta_info.release_buffer(&mainwnd->meta_info, &mainwnd->buf_dsc);
    }

    if (mainwnd->meta_info.on_destroy) {
        mainwnd->meta_info.on_destroy(&mainwnd->meta_info, &mainwnd->buf_dsc);
    }
}

static inline void draw_buffer(lv_obj_t* obj, lv_event_t* e) {
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;

    if (!mainwnd->buf_dsc.img_dsc.data) return;
    if (mainwnd->buf_dsc.img_dsc.header.w == 0 || mainwnd->buf_dsc.img_dsc.header.h == 0) return;

    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    lv_obj_init_draw_image_dsc(obj, LV_PART_MAIN, &img_dsc);
    uint16_t zoom = LV_ZOOM_NONE;

    int32_t img_w = mainwnd->buf_dsc.img_dsc.header.w;
    int32_t img_h = mainwnd->buf_dsc.img_dsc.header.h;

    if (mainwnd->flags & LV_MAINWND_FLAG_DRAW_SCALE) {
        int32_t obj_w = lv_obj_get_width(obj);
        zoom *= obj_w / img_w;
    }
    img_dsc.scale_x = zoom == 0 ? 1 : zoom;
    img_dsc.scale_y = img_dsc.scale_x;
    img_dsc.rotation = 0;
    img_dsc.pivot.x = img_w / 2;
    img_dsc.pivot.y = img_h / 2;
    img_dsc.antialias = 0;

    lv_area_t win_coords, coords;
    lv_obj_get_coords(obj, &coords);
    LV_LOG_TRACE("mainwnd (%d,%d) - (%d,%d)", coords.x1, coords.y1, coords.x2, coords.y2);

    win_coords.x1 = coords.x1;
    win_coords.x2 = win_coords.x1 + img_w - 1;
    win_coords.y1 = coords.y1;
    win_coords.y2 = win_coords.y1 + img_h - 1;

    LV_LOG_INFO("draw (%p) with (%d) (%dx%d), buffer seq=%" PRIu32 "", mainwnd, mainwnd->buf_dsc.id,
                img_w, img_h, mainwnd->buf_dsc.seq);
    img_dsc.src = &mainwnd->buf_dsc.img_dsc;
    lv_draw_image(layer, &img_dsc, &win_coords);
}

static inline void dump_input_event(lv_mainwnd_input_event_t* ie) {
    if (!ie) return;

    LV_LOG_TRACE("input event dump: type(%d), state(%d)", ie->type, ie->state);
    if (ie->type == INPUT_MESSAGE_TYPE_POINTER) {
        LV_LOG_TRACE("\t\traw pos(%d, %d), pos(%d, %d)", ie->pointer.raw_x, ie->pointer.raw_y,
                     ie->pointer.x, ie->pointer.y);

    } else if (ie->type == INPUT_MESSAGE_TYPE_KEYPAD) {
        LV_LOG_TRACE("\t\tkeycode(%d)", ie->keypad.key_code);
    }
}

static inline void send_input_event(lv_mainwnd_t* mainwnd, lv_event_code_t code,
                                    lv_indev_t* indev) {
    lv_mainwnd_input_event_t ie;
    ie.type = lv_indev_get_type(indev);
    LV_LOG_TRACE("mainwnd %p, code %d", mainwnd, code);

    if (code == LV_EVENT_KEY) {
        ie.state = lv_indev_get_state(indev);
        ie.keypad.key_code = lv_indev_get_key(indev);
        dump_input_event(&ie);
        mainwnd->meta_info.send_input_event(&(mainwnd->meta_info), &ie);
    } else {
        if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESSING ||
            code == LV_EVENT_LEAVE) {
            lv_point_t point;
            lv_indev_get_point(indev, &point);

            ie.pointer.raw_x = point.x;
            ie.pointer.raw_y = point.y;
            // raw x, y
            ie.pointer.x = point.x - lv_obj_get_x((lv_obj_t*)mainwnd);
            ie.pointer.y = point.y - lv_obj_get_y((lv_obj_t*)mainwnd);
        } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_PRESS_LOST) {
            ie.pointer.raw_x = lv_obj_get_x((lv_obj_t*)mainwnd) - 10;
            ie.pointer.raw_y = lv_obj_get_y((lv_obj_t*)mainwnd) - 10;
            ie.pointer.x = -10;
            ie.pointer.y = -10;
        } else {
            return;
        }
        ie.pointer.gesture_state = 0;

        if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
            ie.state = LV_INDEV_STATE_PRESSED;
        } else {
            ie.state = LV_INDEV_STATE_RELEASED;
        }

        dump_input_event(&ie);
        mainwnd->meta_info.send_input_event(&(mainwnd->meta_info), &ie);
    }
}

static void lv_mainwnd_event(const lv_obj_class_t* class_p, lv_event_t* e) {
    LV_UNUSED(class_p);

    lv_res_t res;
    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK) return;

    lv_obj_t* obj = lv_event_get_target(e);
    lv_mainwnd_t* mainwnd = (lv_mainwnd_t*)obj;
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
        case LV_EVENT_DRAW_MAIN: {
            if (mainwnd->buf_dsc.id != INVALID_BUFID) {
                draw_buffer(obj, e);
                return;
            }

            if (!mainwnd->meta_info.acquire_buffer) {
                LV_LOG_INFO("no 'acquire_buffer' callback!");
                return;
            }

            if (!mainwnd->meta_info.acquire_buffer(&mainwnd->meta_info, &mainwnd->buf_dsc)) {
                LV_LOG_INFO("acquire_buffer failure!");
                return;
            }
            draw_buffer(obj, e);
            break;
        }

        case LV_EVENT_PRESSED ... LV_EVENT_LEAVE: {
            if (!mainwnd->meta_info.send_input_event) {
                LV_LOG_INFO("no 'send_input_event' callback!");
                return;
            }
            lv_indev_t* indev = lv_event_get_indev(e);
            if (!indev) {
                LV_LOG_INFO("no valid input device!");
                return;
            }
            send_input_event(mainwnd, code, indev);
            break;
        }

        default:
            break;
    }
}
