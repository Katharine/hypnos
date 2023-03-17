//
// Created by Katharine Berry on 3/3/23.
//

#ifndef FIRMWARE_ROOTAPP_H
#define FIRMWARE_ROOTAPP_H

#include <memory>
#include <lvgl_port.h>
#include <backlight.h>
#include "../base_app.h"

namespace apps::root {

class RootApp {
    std::unique_ptr<apps::BaseApp> active_app;
    std::unique_ptr<backlight::Controller> backlight;

public:
    void init();
};

} // root

#endif //FIRMWARE_ROOTAPP_H
