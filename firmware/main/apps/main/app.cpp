#include "app.h"
#include <lvgl.h>

namespace apps::main {

void App::present() {
    showConnectingScreen();
    wifi->startNormal([this](bool x) { staConnectCallback(x); });
}

App::App(const std::shared_ptr<lvgl_port::LVGLPort> &port, const std::shared_ptr<wifi::WiFi> &wifi,
         const std::shared_ptr<hypnos_config::HypnosConfig> &config,
         const std::shared_ptr<eightsleep::Client> &client) : port(port), wifi(wifi), config(config), client(client) {

}

void App::staConnectCallback(bool connected) {
    if (connected) {
        client->authenticate([this](bool success) {
           if (success) {
               showMainScreen();
           } else {
               showConnectionErrorScreen();
           }
        });
    } else {
        showConnectionErrorScreen();
    }
}

void App::showConnectingScreen() {
    auto lock = lvgl_port::Lock();

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Connecting...");
    lv_obj_center(label);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::showMainScreen() {
    auto lock = lvgl_port::Lock();

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "UI goes here.");
    lv_obj_center(label);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::showConnectionErrorScreen() {
    auto lock = lvgl_port::Lock();

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Connection error.");
    lv_obj_center(label);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

}