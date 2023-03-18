//
// Created by Katharine Berry on 3/15/23.
//

#include "menu.h"

#include <lvgl.h>
#include <lvgl_port.h>
#include <statics.h>
#include <prompts.h>

#include <string>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace apps::main::settings {

void Menu::display() {
    // Construct ourselves a menu.

    lv_obj_t *screen = statics::screenStack->createScreen();
    lv_group_t *group = statics::screenStack->groupForScreen(screen);

    lv_group_set_wrap(group, false);

    container = lv_obj_create(screen);
    lv_obj_set_size(container, lvgl_port::LVGLPort::DISPLAY_WIDTH, lvgl_port::LVGLPort::DISPLAY_HEIGHT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *ret = addMenuItem("Return", &handleReturnPressed);
    addMenuItem("Reboot", &handleRebootPressed);
    addMenuItem("Factory Reset", &handleFactoryResetPressed);

    buttonPressed = false;

    // TODO: confirmation prompts.

    statics::screenStack->push(screen);
    lv_obj_scroll_to_view(ret, LV_ANIM_OFF);
}

lv_obj_t* Menu::addMenuItem(const std::string& text, lv_event_cb_t callback) {
    lv_obj_t *button = lv_btn_create(container);
    lv_obj_set_width(button, lv_pct(100));

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text.c_str());

    lv_obj_add_event_cb(button, &handleButtonDown, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(button, callback, LV_EVENT_RELEASED, this);

    return button;
}

void Menu::handleRebootPressed(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    if (!menu->buttonPressed) {
        return;
    }
    prompts::Confirm::Display("Are you\nsure you want to reboot?", "Yes", "No", []() {
        ESP_LOGI("menu", "rebooting!");
        vTaskDelay(1);
        esp_restart();
    }, nullptr);
}

void Menu::handleFactoryResetPressed(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    if (!menu->buttonPressed) {
        return;
    }
    menu->buttonPressed = false;
    prompts::Confirm::Display("Are you\nsure you want to factory reset?", "Yes", "No", []() {
        statics::config->wipe();
        esp_restart();
    }, nullptr);
}

void Menu::handleReturnPressed(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    if (!menu->buttonPressed) {
        return;
    }
    menu->buttonPressed = false;
    statics::screenStack->pop();
}

void Menu::handleButtonDown(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    ESP_LOGI("menu", "button down!");
    menu->buttonPressed = true;
}

}
