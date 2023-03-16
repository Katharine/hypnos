//
// Created by Katharine Berry on 3/15/23.
//

#include "menu.h"

#include <lvgl.h>
#include <lvgl_port.h>

#include <string>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace apps::main::settings {

Menu::Menu(std::shared_ptr<hypnos_config::HypnosConfig> config, std::shared_ptr<lvgl_port::LVGLPort> port, ExitCallback cb) :
    exitCallback(std::move(cb)), config(std::move(config)), port(std::move(port)) {}

void Menu::display() {
    // Construct ourselves a menu.
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_group_set_wrap(group, false);
    port->setActiveGroup(group);
    lv_obj_t *screen = lv_obj_create(nullptr);
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

    // TODO: more of a window stack?
    // TODO: we're leaking these groups everywhere
    // TODO: confirmation prompts.

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, true);
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
    ESP_LOGI("menu", "rebooting!");
    vTaskDelay(100);
    esp_restart();
}

void Menu::handleFactoryResetPressed(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    if (!menu->buttonPressed) {
        return;
    }
    menu->buttonPressed = false;
    menu->config->wipe();
    esp_restart();
}

void Menu::handleReturnPressed(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    if (!menu->buttonPressed) {
        return;
    }
    menu->buttonPressed = false;
    menu->exitCallback();
}

void Menu::handleButtonDown(lv_event_t *event) {
    auto *menu = static_cast<Menu*>(lv_event_get_user_data(event));
    ESP_LOGI("menu", "button down!");
    menu->buttonPressed = true;
}

}