#include "temp_ticks.h"
#include <esp_log.h>

#define MY_CLASS &hb_ticks_class

static const char* TAG = "ticks";

static void hb_ticks_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void hb_ticks_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void hb_ticks_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void hb_ticks_render(hb_ticks_t *ticks, lv_draw_ctx_t *ctx);

const lv_obj_class_t hb_ticks_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = hb_ticks_constructor,
    .destructor_cb = hb_ticks_destructor,
    .event_cb = &hb_ticks_event,
    .width_def = 220,
    .height_def = 220,
    .editable = LV_OBJ_CLASS_EDITABLE_FALSE,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
    .instance_size = sizeof(hb_ticks_t),
};

lv_obj_t *hb_ticks_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


void hb_ticks_set_rotation(lv_obj_t *obj, int16_t rotation) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    ((hb_ticks_t *)obj)->rotation = rotation;

    lv_obj_invalidate(obj);
}

void hb_ticks_set_angles(lv_obj_t *obj, int16_t start, int16_t end) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;
    ticks->start_angle = start;
    ticks->end_angle = end;

    lv_obj_invalidate(obj);
}

void hb_ticks_set_range(lv_obj_t *obj, int16_t start, int16_t end) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;
    ticks->range_start = start;
    ticks->range_end = end;

    lv_obj_invalidate(obj);
}

void hb_ticks_set_true_target(lv_obj_t *obj, int16_t target) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (ticks->true_target == target && ticks->has_true_target) {
        return;
    }

    ticks->true_target = target;
    ticks->has_true_target = true;

    lv_obj_invalidate(obj);
}

void hb_ticks_clear_true_target(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (!ticks->has_true_target) {
        return;
    }

    ticks->has_true_target = false;

    lv_obj_invalidate(obj);
}

void hb_ticks_set_requested_target(lv_obj_t *obj, int16_t target) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (ticks->requested_target == target && ticks->has_requested_target) {
        return;
    }

    ticks->requested_target = target;
    ticks->has_requested_target = true;

    lv_obj_invalidate(obj);
}

void hb_ticks_clear_requested_target(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (!ticks->has_requested_target) {
        return;
    }

    ticks->has_requested_target = false;

    lv_obj_invalidate(obj);
}

void hb_ticks_set_actual(lv_obj_t *obj, int16_t actual) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (ticks->actual == actual && ticks->has_actual) {
        return;
    }

    ticks->actual = actual;
    ticks->has_actual = true;
    lv_obj_invalidate(obj);
}

void hb_ticks_clear_actual(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    if (!ticks->has_actual) {
        return;
    }

    ticks->has_actual = false;
}

void hb_ticks_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    LV_UNUSED(class_p);
    hb_ticks_t *ticks = (hb_ticks_t *)obj;

    ticks->end_angle = 360;
}

static void hb_ticks_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    LV_UNUSED(class_p);
}

void hb_ticks_event(const lv_obj_class_t *class_p, lv_event_t *e) {
    LV_UNUSED(class_p);

    lv_res_t res;
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_MAIN_END) {
        res = lv_obj_event_base(MY_CLASS, e);
        if (res != LV_RES_OK) {
            return;
        }
    }

    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_DRAW_MAIN) {
        hb_ticks_render((hb_ticks_t *)obj, lv_event_get_draw_ctx(e));
    }
}

static inline bool prv_is_between(int value, int start, int end) {
    return (start <= end && value >= start && value <= end) || (start > end && value >= end && value <= start);
}

void hb_ticks_render(hb_ticks_t *ticks, lv_draw_ctx_t *ctx) {
    // If we're invisible, do nothing at all.
    int opa = lv_obj_get_style_opa(&ticks->obj, LV_PART_MAIN);
    if (opa == 0) {
        return;
    }

    // All of these numbers are left shifted by three for increased precision.
    // Remember to undo this before handing them off to the drawing routines.
    int arc_angle = (ticks->end_angle - ticks->start_angle) << 3;
    int degrees_per_tick = arc_angle / (ticks->range_start - ticks->range_end);
    int centre_x = (((ticks->obj.coords.x1 + ticks->obj.coords.x2))) << 2;
    int centre_y = ((ticks->obj.coords.y1 + ticks->obj.coords.y2)) << 2;
    int radius = (lv_obj_get_width(&ticks->obj)) << 2;
    int inner_radius = radius - (10 << 3);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);

    line_dsc.color = lv_color_white();
    line_dsc.width = 3;

    // Range is doubled, so we can do halfway marks as appropriate
    for (int i = ticks->range_start * 2, j = 0; i <= ticks->range_end * 2; ++i, ++j) {
        int tick_inner_radius = inner_radius;
        if (ticks->has_true_target && ticks->has_requested_target && ticks->true_target != ticks->requested_target && i == ticks->true_target * 2) {
            // If this is the true target, and the true target is not the requested target, highlight it.
            line_dsc.color = lv_color_hex(0xf7c93e);
            tick_inner_radius -= (5 << 3);
        } else if (ticks->has_requested_target && ticks->has_actual && prv_is_between(i, ticks->requested_target * 2, ticks->actual * 2)) {
            // If this tick is between the requested and actual values, draw highlighted half ticks
            line_dsc.color = lv_color_white();
            if (i == ticks->requested_target * 2 || i == ticks->actual * 2) {
                tick_inner_radius -= (5 << 3);
            }
        } else {
            if (j % 2 == 1) {
                // Skip all the halfway ticks.
                continue;
            }
            line_dsc.color = lv_color_make(128, 128, 128);
        }
        lv_coord_t start_x = (tick_inner_radius * lv_trigo_cos(ticks->rotation - ((j * degrees_per_tick) >> 4))) >> LV_TRIGO_SHIFT;
        lv_coord_t start_y = (tick_inner_radius * lv_trigo_sin(ticks->rotation - ((j * degrees_per_tick) >> 4))) >> LV_TRIGO_SHIFT;
        lv_coord_t end_x = (radius * lv_trigo_cos(ticks->rotation - ((j * degrees_per_tick) >> 4))) >> LV_TRIGO_SHIFT;
        lv_coord_t end_y = (radius * lv_trigo_sin(ticks->rotation - ((j * degrees_per_tick) >> 4))) >> LV_TRIGO_SHIFT;
        lv_draw_line(ctx, &line_dsc, &((lv_point_t){.x = (centre_x + start_x) >> 3, .y = (centre_y + start_y) >> 3}), &((lv_point_t){.x = (centre_x + end_x) >> 3, .y = (centre_y + end_y) >> 3}));
    }
}