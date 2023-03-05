#ifndef COMPONENT_WIFI_WIFI_H
#define COMPONENT_WIFI_WIFI_H

#include <string>
#include <esp_wifi_types.h>
#include <functional>
#include <esp_netif_types.h>

namespace wifi {

class WiFi {
public:
    WiFi();
    void startNormal(const std::function<void(bool)>& connectCallback);
    void startAP();
    void joinNetwork(const std::string& ssid, const std::string& psk);
    std::string getAPSSID();
    bool hasConfig();
    std::string getConfiguredSSID();
    void setAPConnectCallback(std::function<void(bool)> callback);
    void setSTAConnectCallback(std::function<void(bool)> callback);
    void setScanCallback(const std::function<void(const std::vector<wifi_ap_record_t>&)>& fn);
    std::string getAPIP();
    void startScan();
private:
    std::function<void(bool)> apConnectCallback;
    std::function<void(bool)> staConnectCallback;
    std::function<void(const std::vector<wifi_ap_record_t>&)> scanCallback;
    esp_netif_obj *apNetif;
    esp_netif_obj *staNetif;
    bool hasClients = false;

    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

}

#endif
