//
// Created by Katharine Berry on 3/3/23.
//

#include "root_app.h"

#include <lvgl_port.h>
#include <wifi.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <styles.h>
#include <statics.h>
#include "../config/app.h"
#include "../main/app.h"

#include <memory>

namespace apps::root {

void RootApp::init() {
    lvgl_port::LVGLPort::resetDisplay();
    ESP_LOGI("wifi", "Initialising NVS...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI("wifi", "Initialising event loop...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    statics::port = std::make_unique<lvgl_port::LVGLPort>();
    backlight = std::make_unique<backlight::Controller>();
    statics::wifi = std::make_unique<wifi::WiFi>();
    statics::config = std::make_unique<hypnos_config::HypnosConfig>();
    statics::client = std::make_unique<eightsleep::Client>();

    // Set up our shared styles/theming.
    styles::init();

    if (statics::wifi->hasConfig() && statics::config->hasConfig()) {
        statics::client->setLogin(statics::config->getEmail(), statics::config->getPassword());
        ESP_LOGI("App", "Jumping straight into the main app!");
        active_app = std::make_unique<apps::main::App>();
        active_app->present();
    } else {
        // Start config app
        ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        active_app = std::make_unique<apps::config::App>();
        active_app->present();
        ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        // Now we can start the main app anyway.
        ESP_LOGI("App", "Time to launch the main app!");
        active_app = std::make_unique<apps::main::App>();
        active_app->present();
    }
}

} // root