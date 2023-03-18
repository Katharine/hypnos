//
// Created by Katharine Berry on 3/16/23.
//

#include "statics.h"

std::unique_ptr<lvgl_port::LVGLPort> statics::port = nullptr;
std::unique_ptr<wifi::WiFi> statics::wifi = nullptr;
std::unique_ptr<hypnos_config::HypnosConfig> statics::config = nullptr;
std::unique_ptr<eightsleep::Client> statics::client = nullptr;
std::unique_ptr<screens::Stack> statics::screenStack = std::make_unique<screens::Stack>();
