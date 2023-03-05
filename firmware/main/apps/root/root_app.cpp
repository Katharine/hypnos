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
#include "../config/app.h"

#include <memory>

namespace apps::root {

void RootApp::init() {
    ESP_LOGI("wifi", "Initialising NVS...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI("wifi", "Initialising event loop...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    port = std::make_shared<lvgl_port::LVGLPort>();
    wifi = std::make_shared<wifi::WiFi>();
    config = std::make_shared<hypnos_config::HypnosConfig>();
    client = std::make_shared<eightsleep::Client>();

    // Set up our shared styles/theming.
    styles::init();

    if (wifi->hasConfig() && config->hasConfig()) {
        ESP_LOGI("App", "Jumping straight into the main app!");
    } else {
        // Start config app
        ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        active_app = std::make_unique<apps::config::App>(port, wifi, config, client);
        active_app->present();
        ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        // Now we can start the main app anyway.
        ESP_LOGI("App", "Time to launch the main app!");
    }
}

} // root