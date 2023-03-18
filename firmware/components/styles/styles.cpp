//
// Created by Katharine Berry on 3/3/23.
//

#include "styles.h"
#include <lvgl.h>
#include <wifi_widget.h>

static lv_theme_t custom_theme;
static lv_style_t screen_style;
static lv_style_t wifi_style;
static lv_style_t container_style;

namespace {


void theme_apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
    LV_UNUSED(theme);
    // It's a screen
    if (lv_obj_get_parent(obj) == nullptr) {
        lv_obj_add_style(obj, &screen_style, 0);
        return;
    }
    if (lv_obj_get_class(obj) == &lv_obj_class) {
        lv_obj_add_style(obj, &container_style, 0);
        return;
    }
    if (lv_obj_get_class(obj) == &hb_wifi_class) {
        LV_LOG_ERROR("setting wifi style...");
        lv_obj_add_style(obj, &wifi_style, LV_PART_MAIN);
    }
}

}

void styles::init() {
    // Set up the screen theme
    lv_style_init(&screen_style);
    lv_style_set_bg_color(&screen_style, lv_color_black());
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);

    // Set up the container style
    lv_style_init(&container_style);
    lv_style_set_bg_color(&container_style, lv_color_black());
    lv_style_set_bg_opa(&container_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&container_style, 0);
//    lv_style_set_pad_all(&container_style, 0);

    // Set up the Wi-Fi style
    lv_style_init(&wifi_style);
    lv_style_set_arc_color(&wifi_style, lv_color_white());
    lv_style_set_arc_opa(&wifi_style, LV_OPA_100);
    // This is meaningless, but if it's not set we never set the colour either...
    lv_style_set_arc_width(&wifi_style, 10);

    // Extend the default theme
    lv_theme_t *default_theme = lv_disp_get_theme(nullptr);
    custom_theme = *default_theme;
    lv_theme_set_parent(&custom_theme, default_theme);
    lv_theme_set_apply_cb(&custom_theme, &theme_apply_cb);
    lv_disp_set_theme(nullptr, &custom_theme);
}