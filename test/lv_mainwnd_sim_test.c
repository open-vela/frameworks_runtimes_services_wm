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

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int id;
    lv_coord_t w;
    lv_coord_t h;
    uint32_t cf : 5;
    uint8_t data_type : 2;
    const void * info;
} lv_sim_meta_info_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool lv_mainwnd_acquire_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc);
static bool lv_mainwnd_release_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc);
static bool lv_mainwnd_send_input_event(lv_mainwnd_metainfo_t *meta,
                                       lv_mainwnd_input_event_t *event);
static bool lv_mainwnd_on_destroy(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t* buf_dsc);

static void lv_sim_metainfo_generate(lv_sim_meta_info_t *sim_meta_info, const void *info, int id,
                                     lv_coord_t w, lv_coord_t h, uint32_t cf);

static void lv_metainfo_init(lv_mainwnd_metainfo_t *meta_info);
static void lv_set_sim_metainfo(lv_mainwnd_metainfo_t *meta_info,
                                lv_sim_meta_info_t *sim_meta_info);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_mainwnd_sim_test(void)
{
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(BUFFER_WIDTH, BUFFER_HEIGHT)];

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 5;
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_HOR;
    rect_dsc.bg_grad.stops[0].color = lv_palette_main(LV_PALETTE_YELLOW);
    rect_dsc.bg_grad.stops[1].color = lv_palette_main(LV_PALETTE_BLUE);
    rect_dsc.border_width = 2;
    rect_dsc.border_opa = LV_OPA_90;
    rect_dsc.border_color = lv_color_white();
    rect_dsc.shadow_width = 5;
    rect_dsc.shadow_ofs_x = 5;
    rect_dsc.shadow_ofs_y = 5;

    lv_obj_t *lv_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(lv_canvas, cbuf, BUFFER_WIDTH, BUFFER_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(lv_canvas, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_draw_rect(lv_canvas, (BUFFER_WIDTH - 150) / 2, (BUFFER_HEIGHT - 150) / 2, 150, 150,
                        &rect_dsc);
    lv_img_dsc_t *dsc = lv_canvas_get_img(lv_canvas);

    lv_obj_t *app_label = lv_label_create(lv_canvas);
    lv_label_set_text(app_label, "app buffer");

    lv_sim_meta_info_t *sim_meta_info = lv_mem_alloc(sizeof(lv_sim_meta_info_t));
    if (!sim_meta_info) {
        LV_LOG_WARN("lv_mainwnd_sim_test: failed to allocate memory for lv_sim_meta_info_t");
        return;
    }
    lv_sim_metainfo_generate(sim_meta_info, dsc, 2023, BUFFER_WIDTH, BUFFER_HEIGHT,
                             LV_IMG_CF_TRUE_COLOR);

    lv_mainwnd_metainfo_t *meta_info = lv_mem_alloc(sizeof(lv_mainwnd_metainfo_t));
    if (!meta_info) {
        lv_mem_free(sim_meta_info);
        LV_LOG_WARN("lv_mainwnd_sim_test: failed to allocate memory for lv_mainwnd_metainfo_t");
        return;
    }
    lv_metainfo_init(meta_info);
    lv_set_sim_metainfo(meta_info, sim_meta_info);

    lv_obj_t *mainwnd = lv_mainwnd_create(lv_scr_act());
    lv_mainwnd_set_metainfo(mainwnd, meta_info);
    lv_obj_align(mainwnd, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lvgl_label = lv_label_create(mainwnd);
    lv_label_set_text(lvgl_label, "lvgl buffer");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool lv_mainwnd_acquire_buffer(lv_mainwnd_metainfo_t * meta, lv_mainwnd_buf_dsc_t * buf_dsc)
{
    if (!buf_dsc || !meta) {
        return false;
    }

    lv_sim_meta_info_t * sim_meta_info = (lv_sim_meta_info_t *)(meta->info);
    buf_dsc->id = sim_meta_info->id;
    buf_dsc->w = sim_meta_info->w;
    buf_dsc->h = sim_meta_info->h;
    buf_dsc->data = (void *)sim_meta_info->info;
    buf_dsc->cf = sim_meta_info->cf;

    return true;
}

static bool lv_mainwnd_release_buffer(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t *buf_dsc) {
    // TODO
    return true;
}

static bool lv_mainwnd_send_input_event(lv_mainwnd_metainfo_t *meta,
                                       lv_mainwnd_input_event_t *event) {
    // TODO
    return true;
}

static bool lv_mainwnd_on_destroy(lv_mainwnd_metainfo_t *meta, lv_mainwnd_buf_dsc_t* buf_dsc) {
    // TODO
    return true;
}

static void lv_sim_metainfo_generate(lv_sim_meta_info_t *sim_meta_info, const void *info, int id,
                                     lv_coord_t w, lv_coord_t h, uint32_t cf) {
    sim_meta_info->info = info;
    sim_meta_info->id = id;
    sim_meta_info->w = w;
    sim_meta_info->h = h;
    sim_meta_info->cf = cf;
}

static void lv_metainfo_init(lv_mainwnd_metainfo_t * meta_info)
{
    meta_info->acquire_buffer = lv_mainwnd_acquire_buffer;
    meta_info->release_buffer = lv_mainwnd_release_buffer;
    meta_info->send_input_event = lv_mainwnd_send_input_event;
    meta_info->on_destroy = lv_mainwnd_on_destroy;
}

static void lv_set_sim_metainfo(lv_mainwnd_metainfo_t *meta_info,
                                lv_sim_meta_info_t *sim_meta_info) {
    meta_info->info = sim_meta_info;
}