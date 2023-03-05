//
// Created by Katharine Berry on 3/3/23.
//

#include "styles.h"
#include <lvgl.h>

static lv_theme_t custom_theme;
static lv_style_t screen_style;

namespace {

void theme_apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    LV_UNUSED(theme);
    // It's a screen
    if (lv_obj_get_parent(obj) == nullptr) {
        lv_obj_add_style(obj, &screen_style, 0);
    }
}

}

void styles::init() {
    // Set up the screen theme
    lv_style_init(&screen_style);
    lv_style_set_bg_color(&screen_style, lv_color_black());
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);

    // Extend the default theme
    lv_theme_t *default_theme = lv_disp_get_theme(nullptr);
    custom_theme = *default_theme;
    lv_theme_set_parent(&custom_theme, default_theme);
    lv_theme_set_apply_cb(&custom_theme, &theme_apply_cb);
    lv_disp_set_theme(nullptr, &custom_theme);
}