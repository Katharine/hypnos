#include "prompts.h"

#include <utility>
#include <lvgl.h>
#include <statics.h>

namespace prompts {

Confirm::Confirm(std::string message, std::string positiveText, std::string negativeText, ConfirmCallback positive,
                 ConfirmCallback negative) :
                 message(std::move(message)), positiveText(std::move(positiveText)),
                 negativeText(std::move(negativeText)), positiveCallback(std::move(positive)),
                 negativeCallback(std::move(negative)) {

}

void Confirm::display() {
    lv_obj_t *screen = statics::screenStack->createScreen();

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, message.c_str());
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_align(label, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(label, 0, 20);
    lv_obj_set_size(label, 200, 200);
    lv_obj_set_style_radius(label, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *no = lv_btn_create(screen);
    lv_obj_set_align(no, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(no, -50, -40);
    lv_obj_t *no_label = lv_label_create(no);
    lv_label_set_text(no_label, negativeText.c_str());

    lv_obj_t *yes = lv_btn_create(screen);
    lv_obj_set_align(yes, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(yes, 50, -40);
    lv_obj_t *yes_label = lv_label_create(yes);
    lv_label_set_text(yes_label, positiveText.c_str());

    lv_obj_add_event_cb(no, [](lv_event_t *evt) {
        auto *confirm = static_cast<Confirm*>(lv_event_get_user_data(evt));
        statics::screenStack->pop();
        if (confirm->negativeCallback) {
            confirm->negativeCallback();
        }
        delete confirm;
    }, LV_EVENT_RELEASED, this);

    lv_obj_add_event_cb(yes, [](lv_event_t *evt) {
        auto *confirm = static_cast<Confirm*>(lv_event_get_user_data(evt));
        statics::screenStack->pop();
        if (confirm->positiveCallback) {
            confirm->positiveCallback();
        }
        delete confirm;
    }, LV_EVENT_RELEASED, this);

    statics::screenStack->push(screen);
}

void Confirm::Display(std::string message, std::string positiveText, std::string negativeText, ConfirmCallback positive,
                      ConfirmCallback negative) {
   auto* confirm = new Confirm(std::move(message), std::move(positiveText), std::move(negativeText), std::move(positive), std::move(negative));
   confirm->display();
}


}