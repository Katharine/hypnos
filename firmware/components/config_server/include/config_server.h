#ifndef COMPONENT_CONFIG_SERVER_CONFIG_SERVER_H
#define COMPONENT_CONFIG_SERVER_CONFIG_SERVER_H

#include <esp_http_server.h>
#include <memory>
#include <wifi.h>

typedef void* httpd_handle_t;

namespace config_server {

class Server {
    std::shared_ptr<wifi::WiFi> wifi;
public:
    Server(const std::shared_ptr<wifi::WiFi>& wifi);
    ~Server();

private:
    httpd_handle_t httpd;
    static int serveIndex(httpd_req_t *req);
    static int serveScan(httpd_req_t *req);
    static esp_err_t serveJoin(httpd_req_t *req);
    esp_err_t  sendSocketData(int fd, const std::string& data);
};

}

#endif
