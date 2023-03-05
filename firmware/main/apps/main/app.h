#ifndef FIRMWARE_MAIN_APP_H
#define FIRMWARE_MAIN_APP_H

#include <eightsleep.h>
#include <config_server.h>
#include <lvgl_port.h>

#include "../base_app.h"

namespace apps::main {

class App : public BaseApp {
    std::shared_ptr<lvgl_port::LVGLPort> port;
    std::shared_ptr<wifi::WiFi> wifi;
    std::shared_ptr<hypnos_config::HypnosConfig> config;
    std::unique_ptr<config_server::Server> server;
    std::shared_ptr<eightsleep::Client> client;

public:
    App(const std::shared_ptr<lvgl_port::LVGLPort> &port, const std::shared_ptr<wifi::WiFi> &wifi, const std::shared_ptr<hypnos_config::HypnosConfig> &config, const std::shared_ptr<eightsleep::Client>& client);
    void present() override;

private:
    void staConnectCallback(bool connected);
    void showConnectingScreen();
    void showConnectionErrorScreen();
    void showMainScreen();
};

}

#endif
