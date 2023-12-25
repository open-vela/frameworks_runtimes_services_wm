#include "lvgl/lvgl.h"

static void lv_obj_set_scale(void *var, int32_t v) {
    lv_obj_set_style_transform_scale(var, v, LV_PART_MAIN);
}

static void lv_obj_set_opa(void *var, int32_t v) {
    lv_obj_set_style_opa(var, v, LV_PART_MAIN);
}

int main(void) {
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;

    lv_init();
    lv_nuttx_dsc_init(&info);

    lv_nuttx_init(&info, &result);

    if (result.disp == NULL) {
        LV_LOG_ERROR("lv_demos initialization failure!");
        return 1;
    }

    lv_obj_t *img = lv_img_create(lv_screen_active());
    lv_img_set_src(img, "/data/app/logo.png");
    lv_obj_set_pos(img, 50, 50);
    lv_obj_set_size(img, 100, 100);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, img);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_scale);
    lv_anim_set_values(&anim, LV_ZOOM_NONE, LV_ZOOM_NONE + 256);
    lv_anim_set_time(&anim, 1000);
    lv_anim_set_playback_time(&anim, 500);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim);

    lv_anim_init(&anim);
    lv_anim_set_var(&anim, img);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_opa);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim, 1000);
    lv_anim_set_playback_time(&anim, 500);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim);

    while (1) {
        lv_timer_handler();
        usleep(10 * 1000);
    }

    lv_disp_remove(result.disp);
    lv_deinit();

    return 0;
}
