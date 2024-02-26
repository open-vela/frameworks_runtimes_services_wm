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
 * @file lv_mainwnd_sim_test.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../server/lvgl/lv_mainwnd.h"

/*********************
 *      DEFINES
 *********************/

#define BUFFER_WIDTH 200
#define BUFFER_HEIGHT 200
#define CANVAS_RECT_SIDE 150

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int id;
    int32_t w;
    int32_t h;
    uint32_t cf : 5;
    uint8_t data_type : 2;
    const void *info;
} lv_sim_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool acquire_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc);
static bool release_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc);
static bool send_input_event(lv_mainwnd_metainfo_t *meta, lv_mainwnd_input_event_t *event);
static bool on_destroy(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc);

static void metainfo_init(lv_mainwnd_metainfo_t *meta_info);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_mainwnd_sim_test(void) {
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(BUFFER_WIDTH, BUFFER_HEIGHT)];

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 5;
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_HOR;
    rect_dsc.bg_grad.stops[0].color = lv_palette_main(LV_PALETTE_YELLOW);
    rect_dsc.bg_grad.stops[1].color = lv_palette_main(LV_PALETTE_BLUE);
    rect_dsc.bg_grad.stops[0].opa = LV_OPA_COVER;
    rect_dsc.bg_grad.stops[1].opa = LV_OPA_COVER;
    rect_dsc.border_width = 2;
    rect_dsc.border_opa = LV_OPA_90;
    rect_dsc.border_color = lv_color_white();
    rect_dsc.shadow_width = 5;
    rect_dsc.shadow_ofs_x = 5;
    rect_dsc.shadow_ofs_y = 5;

    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_obj_align(canvas, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(canvas, cbuf, BUFFER_WIDTH, BUFFER_HEIGHT,
                         LV_COLOR_FORMAT_NATIVE_WITH_ALPHA);
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_area_t coords = {(BUFFER_WIDTH - CANVAS_RECT_SIDE) / 2,
                        (BUFFER_HEIGHT - CANVAS_RECT_SIDE) / 2, CANVAS_RECT_SIDE, CANVAS_RECT_SIDE};

    lv_draw_rect(&layer, &rect_dsc, &coords);

    lv_canvas_finish_layer(canvas, &layer);

    lv_obj_t *app_label = lv_label_create(canvas);
    lv_label_set_text(app_label, "app buffer");

    lv_sim_t *sim_data = lv_malloc(sizeof(lv_sim_t));
    if (!sim_data) {
        LV_LOG_WARN("lv_mainwnd_sim_test: failed to allocate memory for lv_sim_t");
        return;
    }

    sim_data->info = cbuf;
    sim_data->id = 2023;
    sim_data->w = BUFFER_WIDTH;
    sim_data->h = BUFFER_HEIGHT;
    sim_data->cf = LV_COLOR_FORMAT_NATIVE_WITH_ALPHA;
    lv_mainwnd_metainfo_t *meta_info = lv_malloc(sizeof(lv_mainwnd_metainfo_t));
    if (!meta_info) {
        lv_free(sim_data);
        LV_LOG_WARN("lv_mainwnd_sim_test: failed to allocate memory for lv_mainwnd_metainfo_t");
        return;
    }
    metainfo_init(meta_info);
    meta_info->info = sim_data;

    lv_obj_t *mainwnd = lv_mainwnd_create(lv_scr_act());
    lv_mainwnd_set_metainfo(mainwnd, meta_info);
    lv_obj_align(mainwnd, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lvgl_label = lv_label_create(mainwnd);
    lv_label_set_text(lvgl_label, "lvgl buffer");
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool acquire_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc) {
    if (!buf_dsc || !meta) {
        return false;
    }

    lv_sim_t *sim_data = (lv_sim_t *)(meta->info);
    buf_dsc->id = sim_data->id;
    buf_dsc->img_dsc.data = sim_data->info;
    buf_dsc->img_dsc.data_size = LV_IMAGE_BUF_SIZE_ALPHA_4BIT(sim_data->w, sim_data->h);
    buf_dsc->img_dsc.header.w = sim_data->w;
    buf_dsc->img_dsc.header.h = sim_data->h;
    buf_dsc->img_dsc.header.cf = sim_data->cf;

    return true;
}

static bool release_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc) {
    // TODO
    return true;
}

static bool send_input_event(lv_mainwnd_metainfo_t *meta, lv_mainwnd_input_event_t *event) {
    // TODO
    return true;
}

static bool on_destroy(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc) {
    // TODO
    return true;
}

static void metainfo_init(lv_mainwnd_metainfo_t *meta_info) {
    meta_info->acquire_buffer = acquire_buffer;
    meta_info->release_buffer = release_buffer;
    meta_info->send_input_event = send_input_event;
    meta_info->on_destroy = on_destroy;
}
