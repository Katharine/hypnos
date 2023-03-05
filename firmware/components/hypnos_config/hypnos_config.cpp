#include "hypnos_config.h"

#include <nvs_handle.hpp>
#include <esp_err.h>

namespace hypnos_config {

HypnosConfig::HypnosConfig() {
    esp_err_t err;
    handle = nvs::open_nvs_handle("hypnos", NVS_READWRITE, &err);
    ESP_ERROR_CHECK(err);
}

bool HypnosConfig::hasConfig() {
    int configVersion;
    esp_err_t err = handle->get_item("storage_version", configVersion);
    switch (err) {
        case ESP_OK:
            return true;
        case ESP_ERR_NVS_NOT_FOUND:
            return false;
        default:
            ESP_ERROR_CHECK(err);
    }
    __builtin_unreachable();
}

}
