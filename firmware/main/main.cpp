#include <lvgl.h>
#include <lvgl_port.h>

static lv_obj_t * slider_label;
static lvgl_port::LVGLPort *port;

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%ld%%", lv_slider_get_value(slider));
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    port->setBacklight(lv_slider_get_value(slider));
}

extern "C" void app_main(void) {
    port = new lvgl_port::LVGLPort();

    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    port->setActiveGroup(g);

    lv_obj_t * spinner = lv_spinner_create(lv_scr_act(), 1000, 60);
    lv_obj_set_size(spinner, 240, 240);
    lv_obj_center(spinner);

    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 200, 20);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /*Create a label below the slider*/
    slider_label = lv_label_create(lv_scr_act());
    lv_label_set_text(slider_label, "0%");

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

}
