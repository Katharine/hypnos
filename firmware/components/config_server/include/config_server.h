#ifndef COMPONENT_CONFIG_SERVER_CONFIG_SERVER_H
#define COMPONENT_CONFIG_SERVER_CONFIG_SERVER_H

#include <esp_http_server.h>
#include <memory>
#include <functional>

typedef void* httpd_handle_t;

namespace config_server {

class Server {
    std::function<void(bool)> completionCallback;
public:
    Server();
    ~Server();
    void setCompletionCallback(const std::function<void(bool)>& fn);

private:
    httpd_handle_t httpd = nullptr;
    static esp_err_t serveIndex(httpd_req_t *req);
    static esp_err_t serveScan(httpd_req_t *req);
    static esp_err_t serveJoin(httpd_req_t *req);
    static esp_err_t serveLogIn(httpd_req_t *req);
    esp_err_t  sendSocketData(int fd, const std::string& data);
    void complete(const std::string& email, const std::string& password);
};

}

#endif
