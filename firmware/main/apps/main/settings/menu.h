//
// Created by Katharine Berry on 3/15/23.
//

#ifndef MAIN_APPS_MAIN_MENU_H
#define MAIN_APPS_MAIN_MENU_H

#include <functional>
#include <string>
#include <memory>
#include <hypnos_config.h>
#include <lvgl.h>
#include <lvgl_port.h>

namespace apps::main::settings {

typedef std::function<void()> ExitCallback;

class Menu {
    ExitCallback exitCallback;
    lv_obj_t *container = nullptr;
    std::shared_ptr<hypnos_config::HypnosConfig> config;
    std::shared_ptr<lvgl_port::LVGLPort> port;
    bool buttonPressed = false;

public:
    explicit Menu(std::shared_ptr<hypnos_config::HypnosConfig> config, std::shared_ptr<lvgl_port::LVGLPort> port, ExitCallback cb);
    void display();
    ~Menu() = default;

private:
    lv_obj_t *addMenuItem(const std::string& text, lv_event_cb_t callback);
    static void handleReturnPressed(lv_event_t *event);
    static void handleRebootPressed(lv_event_t *event);
    static void handleFactoryResetPressed(lv_event_t *event);
    static void handleButtonDown(lv_event_t *event);
};

}

#endif // MAIN_APPS_MAIN_MENU_H
