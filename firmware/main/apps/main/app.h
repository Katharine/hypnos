#ifndef FIRMWARE_MAIN_APP_H
#define FIRMWARE_MAIN_APP_H

#include <eightsleep.h>
#include <config_server.h>
#include <lvgl_port.h>

#include "../base_app.h"
#include "settings/menu.h"
#include "state.h"

namespace apps::main {

enum struct Screen {
    Connecting,
    ConnectionError,
    FetchingState,
    Main,
    Alarm,
    Settings,
};

class App : public BaseApp {
    std::unique_ptr<config_server::Server> server;
    settings::Menu menu;
    Screen currentScreen;
    esp_timer_handle_t longPressTimer = nullptr;

    StateManager stateManager{};

public:
    void present() override;

private:
    void staConnectCallback(bool connected);
    void showConnectingScreen();
    void showConnectionErrorScreen();
    void showMainScreen();
    void showAlarmScreen();
    void updateMainScreen();
    void handleStateUpdate(const State& state);
    void showFetchingStateScreen();

    // main screen UI objects
    lv_obj_t *settingArc = nullptr;
    lv_obj_t *tempLabel = nullptr;
    lv_obj_t *tempDescLabel = nullptr;
    lv_obj_t *tickCircle = nullptr;
};

}

#endif
