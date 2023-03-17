//
// Created by Katharine Berry on 3/15/23.
//

#ifndef MAIN_APPS_MAIN_MENU_H
#define MAIN_APPS_MAIN_MENU_H

#include <string>
#include <lvgl.h>

namespace apps::main::settings {

class Menu {
    lv_obj_t *container = nullptr;
    bool buttonPressed = false;

public:
    void display();

private:
    lv_obj_t *addMenuItem(const std::string& text, lv_event_cb_t callback);
    static void handleReturnPressed(lv_event_t *event);
    static void handleRebootPressed(lv_event_t *event);
    static void handleFactoryResetPressed(lv_event_t *event);
    static void handleButtonDown(lv_event_t *event);
};

}

#endif // MAIN_APPS_MAIN_MENU_H
