#include "wifi_widget.h"
#include <esp_log.h>

#define MY_CLASS &hb_wifi_class

static void hb_wifi_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void hb_wifi_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void hb_wifi_event(const lv_obj_class_t * class_p, lv_event_t * e);


const lv_obj_class_t hb_wifi_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = &hb_wifi_constructor,
    .destructor_cb = &hb_wifi_destructor,
    .event_cb = &hb_wifi_event,
    .width_def = 140,
    .height_def = 100,
    .editable = LV_OBJ_CLASS_EDITABLE_FALSE,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
    .instance_size = sizeof(hb_wifi_t),
};

lv_obj_t *hb_wifi_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void hb_wifi_set_active_bar(lv_obj_t *obj, uint32_t bar) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    hb_wifi_t *wifi = (hb_wifi_t *) obj;
    wifi->highlighted_bar = bar;
    lv_obj_invalidate(obj);
}

void hb_wifi_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    hb_wifi_t *wifi = (hb_wifi_t *) obj;

    wifi->highlighted_bar = 0;
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t) hb_wifi_set_active_bar);
    lv_anim_set_var(&anim, wifi);
    lv_anim_set_time(&anim, 1500);
    lv_anim_set_values(&anim, 0, 2);
    lv_anim_set_repeat_delay(&anim, 750);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim);
}

void hb_wifi_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    hb_wifi_t *wifi = (hb_wifi_t *) obj;
    lv_anim_del(&wifi, (lv_anim_exec_xcb_t) hb_wifi_set_active_bar);
}

void hb_wifi_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res;
    // Call the ancestor's event handler
    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_MAIN_END) {
        res = lv_obj_event_base(MY_CLASS, e);
        if(res != LV_RES_OK) return;
    }

    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_DRAW_MAIN) {
        hb_wifi_t *wifi = (hb_wifi_t *) obj;

        int radius = (obj->coords.y2 - obj->coords.y1) / 3;
        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        lv_obj_init_draw_arc_dsc(obj, LV_PART_MAIN, &arc_dsc);

        lv_color_t highlight_colour = arc_dsc.color;
        lv_color_t dim_colour = lv_color_darken(highlight_colour, 100);

        arc_dsc.rounded = false;
        arc_dsc.width = (lv_coord_t)radius;

        lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

        lv_obj_draw_part_dsc_t  part_draw_dsc;
        lv_obj_draw_dsc_init(&part_draw_dsc, draw_ctx);
        part_draw_dsc.draw_area = &obj->coords;
        part_draw_dsc.class_p = MY_CLASS;
        part_draw_dsc.type = 0; // TODO: an enum I guess
        part_draw_dsc.arc_dsc = &arc_dsc;
        part_draw_dsc.part = LV_PART_MAIN;

        lv_event_send(obj, LV_EVENT_DRAW_PART_BEGIN, &part_draw_dsc);
        lv_point_t centre = {
            .x = (lv_coord_t)((obj->coords.x1 + obj->coords.x2) / 2),
            .y = LV_MAX(obj->coords.y1, obj->coords.y2),
        };

        arc_dsc.color = wifi->highlighted_bar == 0 ? highlight_colour : dim_colour;

        lv_draw_arc(draw_ctx, &arc_dsc, &centre, radius, 270-36, 270+36);
        lv_event_send(obj, LV_EVENT_DRAW_PART_END, &part_draw_dsc);

        arc_dsc.width -= arc_dsc.width / 3;
        radius *= 2;

        lv_event_send(obj, LV_EVENT_DRAW_PART_BEGIN, &part_draw_dsc);
        arc_dsc.color = wifi->highlighted_bar == 1 ? highlight_colour : dim_colour;
        lv_draw_arc(draw_ctx, &arc_dsc, &centre, radius, 270-36, 270+36);
        lv_event_send(obj, LV_EVENT_DRAW_PART_END, &part_draw_dsc);

        radius = (obj->coords.y2 - obj->coords.y1);

        lv_event_send(obj, LV_EVENT_DRAW_PART_BEGIN, &part_draw_dsc);
        arc_dsc.color = wifi->highlighted_bar == 2 ? highlight_colour : dim_colour;
        lv_draw_arc(draw_ctx, &arc_dsc, &centre, radius, 270-36, 270+36);
        lv_event_send(obj, LV_EVENT_DRAW_PART_END, &part_draw_dsc);
    }
}