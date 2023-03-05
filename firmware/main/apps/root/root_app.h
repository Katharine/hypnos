//
// Created by Katharine Berry on 3/3/23.
//

#ifndef FIRMWARE_ROOTAPP_H
#define FIRMWARE_ROOTAPP_H

#include <memory>
#include <lvgl_port.h>
#include <wifi.h>
#include <hypnos_config.h>
#include "../base_app.h"

namespace apps::root {

class RootApp {
    std::shared_ptr<lvgl_port::LVGLPort> port;
    std::shared_ptr<wifi::WiFi> wifi;
    std::shared_ptr<hypnos_config::HypnosConfig> config;
    std::unique_ptr<apps::BaseApp> active_app;

public:
    void init();
};

} // root

#endif //FIRMWARE_ROOTAPP_H
