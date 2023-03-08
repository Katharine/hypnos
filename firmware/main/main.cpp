#include <memory>
#include "apps/root/root_app.h"
#include <esp_log.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_sntp.h>
#include <eightsleep.h>

static std::unique_ptr<apps::root::RootApp> app;

 extern "C" void app_main(void) {
     eightsleep::Client client;

     ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

     sntp_setoperatingmode(SNTP_OPMODE_POLL);
     sntp_setservername(0, "time.apple.com");
     sntp_init();

    app = std::make_unique<apps::root::RootApp>();
    app->init();
}
