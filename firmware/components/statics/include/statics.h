//
// Created by Katharine Berry on 3/16/23.
//

#ifndef STATICS_STATICS_H
#define STATICS_STATICS_H

#include <lvgl_port.h>
#include <wifi.h>
#include <hypnos_config.h>
#include <eightsleep.h>
#include <screens.h>
#include <memory>

namespace statics {

struct Statics {
    std::unique_ptr<lvgl_port::LVGLPort> port = nullptr;
    std::unique_ptr<wifi::WiFi> wifi = nullptr;
    std::unique_ptr<hypnos_config::HypnosConfig> config = nullptr;
    std::unique_ptr<eightsleep::Client> client = nullptr;
    std::unique_ptr<screens::Stack> screenStack = std::make_unique<screens::Stack>();
};

extern Statics statics;

}

#endif // STATICS_STATICS_H
