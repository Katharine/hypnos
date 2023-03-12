#ifndef TEMP_TICKS_TEMP_TICKS_H
#define TEMP_TICKS_TEMP_TICKS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t obj;

    int16_t start_angle;
    int16_t end_angle;
    int16_t range_start;
    int16_t range_end;
    int16_t true_target;
    int16_t requested_target;
    int16_t actual;
    int16_t rotation;
    bool has_true_target;
    bool has_requested_target;
    bool has_actual;
} hb_ticks_t;

extern const lv_obj_class_t hb_ticks_class;

lv_obj_t* hb_ticks_create(lv_obj_t *parent);
void hb_ticks_set_rotation(lv_obj_t *obj, int16_t rotation);
void hb_ticks_set_angles(lv_obj_t *obj, int16_t start, int16_t end);
void hb_ticks_set_range(lv_obj_t *obj, int16_t start, int16_t end);
void hb_ticks_set_true_target(lv_obj_t *obj, int16_t target);
void hb_ticks_clear_true_target(lv_obj_t *obj);
void hb_ticks_set_requested_target(lv_obj_t *obj, int16_t target);
void hb_ticks_clear_requested_target(lv_obj_t *obj);
void hb_ticks_set_actual(lv_obj_t *obj, int16_t actual);
void hb_ticks_clear_actual(lv_obj_t *obj);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // TEMP_TICKS_TEMP_TICKS_H
