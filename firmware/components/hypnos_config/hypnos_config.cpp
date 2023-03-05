#include "hypnos_config.h"

#include <nvs_handle.hpp>
#include <esp_err.h>
#include <vector>

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

void HypnosConfig::storeInitialConfig(const std::string &email, const std::string &password) {
    ESP_ERROR_CHECK(handle->set_string("email", email.c_str()));
    ESP_ERROR_CHECK(handle->set_string("password", password.c_str()));
    ESP_ERROR_CHECK(handle->set_item("storage_version", 1));
}

std::string HypnosConfig::getEmail() {
    size_t size;
    ESP_ERROR_CHECK(handle->get_item_size(nvs::ItemType::SZ, "email", size));
    std::vector<char> chars(size);
    ESP_ERROR_CHECK(handle->get_string("email", chars.data(), chars.size()));
    return chars.data();
}

std::string HypnosConfig::getPassword() {
    size_t size;
    ESP_ERROR_CHECK(handle->get_item_size(nvs::ItemType::SZ, "password", size));
    std::vector<char> chars(size);
    ESP_ERROR_CHECK(handle->get_string("password", chars.data(), chars.size()));
    return chars.data();
}

}
