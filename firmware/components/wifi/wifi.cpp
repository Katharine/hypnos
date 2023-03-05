#include "wifi.h"

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_err.h>
#include <lwip/sys.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <string>
#include <functional>
#include <memory>


using namespace std::literals;

namespace wifi {

WiFi::WiFi() {

    ESP_LOGI("wifi", "Initialising netif...");
    ESP_ERROR_CHECK(esp_netif_init());
    apNetif = esp_netif_create_default_wifi_ap();
    staNetif = esp_netif_create_default_wifi_sta();
    ESP_LOGI("wifi", "Initialising default wifi ap...");
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_LOGI("wifi", "Wifi init...");
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_LOGI("wifi", "Registering event handler...");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi::eventHandler, this, nullptr));

}

void WiFi::startAP() {
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 2,
            .pmf_cfg = {
                .required = false,
            },
        }
    };
    strlcpy(reinterpret_cast<char *>(wifi_config.ap.ssid), getAPSSID().c_str(), 32);

    ESP_LOGI("wifi", "Setting mode...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_LOGI("wifi", "Setting config...");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI("wifi", "Starting wifi...");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("wifi", "AP started");
}

void WiFi::startScan() {
    ESP_ERROR_CHECK(esp_wifi_scan_start(nullptr, false));
}

std::string WiFi::getAPSSID() {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    char mac_chars[5];
    std::snprintf(mac_chars, 5, "%02x%02x", mac[3], mac[4]);
    return "Hypnos-"s + mac_chars;
}

void WiFi::startNormal(const std::function<void(bool)>& fn) {
    ESP_LOGI("wifi", "Connecting to wifi...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    apConnectCallback = fn;
}

bool WiFi::hasConfig() {
    wifi_config_t config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
    return (config.sta.ssid[0] != 0);
}

std::string WiFi::getConfiguredSSID() {
    wifi_config_t config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
    return reinterpret_cast<char*>(config.sta.ssid);
}

void WiFi::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto *wifi = static_cast<WiFi*>(arg);
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("WiFi", "We're connected!");
        if (wifi->staConnectCallback) {
            wifi->staConnectCallback(true);
        }
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI("WiFi", "We're disconnected.");
        if (wifi->staConnectCallback) {
            wifi->staConnectCallback(false);
        }
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        auto* event = static_cast<wifi_event_ap_staconnected_t*>(event_data);
        ESP_LOGI("WiFi", "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        if (!wifi->hasClients && wifi->apConnectCallback) {
            ESP_LOGI("WiFi", "Calling connect callback");
            wifi->apConnectCallback(true);
        }
        wifi->hasClients = true;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        auto* event = static_cast<wifi_event_ap_stadisconnected_t*>(event_data);
        ESP_LOGI("WiFi", "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        wifi_sta_list_t sta_list;
        ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&sta_list));
        if (sta_list.num == 0) {
            wifi->hasClients = false;
            if (wifi->apConnectCallback) {
                ESP_LOGI("WiFi", "Calling disconnect callback");
                wifi->apConnectCallback(false);
            }
        }
    } else if (event_id == WIFI_EVENT_SCAN_DONE) {
        auto *event = static_cast<wifi_event_sta_scan_done_t*>(event_data);
        uint16_t count = event->number;
        std::vector<wifi_ap_record_t> results(count);
        esp_wifi_scan_get_ap_records(&count, results.data());
        if (wifi->scanCallback) {
            wifi->scanCallback(results);
        }
    }
}

void WiFi::setAPConnectCallback(std::function<void(bool)> callback) {
    apConnectCallback = callback;
}

std::string WiFi::getAPIP() {
    esp_netif_ip_info_t info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(apNetif, &info));
    char ip_buf[16] = {0};
    esp_ip4addr_ntoa(&info.ip, ip_buf, 16);
    return ip_buf;
}

void WiFi::setScanCallback(const std::function<void(const std::vector<wifi_ap_record_t> &)>& fn) {
    scanCallback = fn;
}

void WiFi::joinNetwork(const std::string &ssid, const std::string &psk) {
    wifi_config_t config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = 2,
        }
    };
    strlcpy(reinterpret_cast<char *>(config.sta.ssid), ssid.c_str(), 32);
    if (!psk.empty()) {
        strlcpy(reinterpret_cast<char *>(config.sta.password), psk.c_str(), 64);
    }

    ESP_LOGI("wifi", "Setting STA config to connect to %s.", ssid.c_str());
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

    ESP_LOGI("wifi", "Starting connection...");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WiFi::setSTAConnectCallback(std::function<void(bool)> callback) {
    staConnectCallback = std::move(callback);
}

}