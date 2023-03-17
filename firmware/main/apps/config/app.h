//
// Created by Katharine Berry on 3/3/23.
//

#ifndef FIRMWARE_CONFIG_APP_H
#define FIRMWARE_CONFIG_APP_H

#include "../base_app.h"
#include <hypnos_config.h>
#include <config_server.h>
#include <lvgl_port.h>
#include <wifi.h>
#include <lvgl.h>
#include <eightsleep.h>

#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace apps::config {

class App : public BaseApp {
    std::unique_ptr<config_server::Server> server;

    EventGroupHandle_t eventGroup;

public:
    App();
    ~App();

    void present() override;

private:
    void presentWiFiUI();
    void presentLinkUI();
    void wifiConnectCallback(bool connected);
    void completionCallback(bool success);
    lv_obj_t *wifi_screen;
    lv_obj_t *link_screen;
    lv_obj_t *top_label;
    lv_obj_t *wifi_label;
    lv_obj_t *bottom_label;
    lv_obj_t *icon_label;
};

}


#endif //FIRMWARE_CONFIG_APP_H
