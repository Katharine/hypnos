#ifndef WIFI_WIDGET_WIFI_WIDGET_H
#define WIFI_WIDGET_WIFI_WIDGET_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t obj;
    uint32_t highlighted_bar;
} hb_wifi_t;

extern const lv_obj_class_t hb_wifi_class;

lv_obj_t *hb_wifi_create(lv_obj_t *parent);
void hb_wifi_set_active_bar(lv_obj_t *l, uint32_t bar);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
