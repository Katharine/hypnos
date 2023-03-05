#include "config_server.h"

#include <esp_http_server.h>
#include <cmrc/cmrc.hpp>
#include <vector>
#include <ArduinoJson.hpp>
#include <esp_log.h>
#include <string>

using namespace std::string_literals;

CMRC_DECLARE(web);

namespace config_server {

Server::Server(const std::shared_ptr<wifi::WiFi>& wifi, const std::shared_ptr<eightsleep::Client>& client,
               const std::shared_ptr<hypnos_config::HypnosConfig>& config) : wifi(wifi), client(client), config(config) {
    httpd_config_t httpdConfig = HTTPD_DEFAULT_CONFIG();
    httpdConfig.stack_size = 8192; // hack our way out of stack overflow errors.
    ESP_ERROR_CHECK(httpd_start(&httpd, &httpdConfig));
    httpdConfig.uri_match_fn = httpd_uri_match_wildcard;

    // Set up some handlers
    static httpd_uri_t index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = &Server::serveIndex,
    };
    httpd_register_uri_handler(httpd, &index);
    static httpd_uri_t scan = {
        .uri = "/api/scan",
        .method = HTTP_GET,
        .handler = &Server::serveScan,
        .user_ctx = this,
    };
    httpd_register_uri_handler(httpd, &scan);
    static httpd_uri_t join = {
        .uri = "/api/join",
        .method = HTTP_POST,
        .handler = &Server::serveJoin,
        .user_ctx = this,
    };
    httpd_register_uri_handler(httpd, &join);
    static httpd_uri_t login = {
        .uri = "/api/login",
        .method = HTTP_POST,
        .handler = &Server::serveLogIn,
        .user_ctx = this,
    };
    httpd_register_uri_handler(httpd, &login);
}

Server::~Server() {
    httpd_stop(httpd);
}

esp_err_t Server::serveIndex(httpd_req_t *req) {
    auto f = cmrc::web::get_filesystem().open("index.html");
    httpd_resp_send(req, f.begin(), f.end() - f.begin());
    return ESP_OK;
}

esp_err_t Server::serveScan(httpd_req_t *req) {
    ESP_LOGI("server", "Serving scan request...");
    auto *server = static_cast<Server*>(req->user_ctx);
    int fd = httpd_req_to_sockfd(req);
    server->wifi->setScanCallback([server, fd](const std::vector<wifi_ap_record_t>& records) {
        ArduinoJson::DynamicJsonDocument result(2000);
        ESP_LOGI("server", "Got callback...");
        result["success"] = true;
        ArduinoJson::JsonArray array = result.createNestedArray("networks");
        for (const auto& record : records) {
            // Skip enterprise (because I can't be bothered) and WAPI (because what even is that?)
            if (record.authmode == WIFI_AUTH_WPA2_ENTERPRISE || record.authmode == WIFI_AUTH_WAPI_PSK) {
                continue;
            }
            ArduinoJson::JsonObject network = array.createNestedObject();
            network["ssid"] = record.ssid;
            network["authMode"] = record.authmode;
            network["rssi"] = record.rssi;
        }
        struct context {
            std::string serialised;
            int fd;
            Server *server;
        };
        ESP_LOGI("server", "JSON memory usage: %d bytes.", result.memoryUsage());
        auto ctx = new context{.fd = fd, .server = server};
        ArduinoJson::serializeJson(result, ctx->serialised);
        ESP_LOGI("server", "Serialised result: %s", ctx->serialised.c_str());
        httpd_queue_work(server->httpd, [](void *arg) {
            ESP_LOGI("server", "Sending result to client...");
            auto a = static_cast<struct context*>(arg);
            // Annoyingly, the original request is totally invalid by the time we get here.
            // As a result, we are now expected to manually construct a response.
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(a->serialised.length()) + "\r\n\r\n";
            a->server->sendSocketData(a->fd, response);
            a->server->sendSocketData(a->fd, a->serialised);
            delete a;
            ESP_LOGI("server", "Done.");
        }, ctx);
        server->wifi->setScanCallback(nullptr);
    });
    ESP_LOGI("server", "Starting scan...");
    server->wifi->startScan();
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    ESP_LOGI("server", "Waiting...");

    return ESP_OK;
}

esp_err_t Server::sendSocketData(int fd, const std::string &data) {
    const char* buf = data.c_str();
    int buf_len = data.length();

    esp_err_t ret;

    while (buf_len > 0) {
        ret = httpd_socket_send(httpd, fd, buf, buf_len, 0);
        if (ret < 0) {
            ESP_LOGD("server", "error in sendSocketData");
            return ESP_FAIL;
        }
        ESP_LOGD("server", "sent = %d", ret);
        buf     += ret;
        buf_len -= ret;
    }
    return ESP_OK;
}

esp_err_t Server::serveJoin(httpd_req_t *req) {
    auto server = static_cast<Server*>(req->user_ctx);

    std::vector<char> data(128);
    int ret = httpd_req_recv(req, data.data(), data.size());
    if (ret < 0) {
        ESP_LOGW("server", "Reading POST data failed: %d.", ret);
        // Docs say we have to return an error if this happens.
        return ESP_FAIL;
    }
    if (ret == data.size()) {
        // We probably cut off. We aren't actually supporting this - just give up here.
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too large");
        return ESP_OK;
    }

    std::vector<char> ssid(33);
    std::vector<char> psk(64);

    esp_err_t err = httpd_query_key_value(data.data(), "ssid", ssid.data(), ssid.size() - 1);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad SSID");
        return ESP_OK;
    }
    err = httpd_query_key_value(data.data(), "psk", psk.data(), psk.size() - 1);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad PSK");
        return ESP_OK;
    }

    // If we got this far, we're actually doing something.
    // Stash this away so we can respond asynchronously.
    int fd = httpd_req_to_sockfd(req);
    server->wifi->setSTAConnectCallback([=](bool connected) {
        struct context {
            Server *server;
            int fd;
            bool connected;
        };
        auto *ctx = new context {
            .server = server,
            .fd = fd,
            .connected = connected
        };
        httpd_queue_work(server->httpd, [](void *arg) {
            auto *ctx = static_cast<context*>(arg);
            std::string json = R"({"success": )"s + (ctx->connected ? "true"s : "false"s) + "}"s;
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(json.length()) + "\r\n\r\n";
            ctx->server->sendSocketData(ctx->fd, response);
            ctx->server->sendSocketData(ctx->fd, json);
        }, ctx);
        server->wifi->setSTAConnectCallback(nullptr);
    });
    server->wifi->joinNetwork(http::urlDecode(ssid.data()), http::urlDecode(psk.data()));
    return ESP_OK;
}

esp_err_t Server::serveLogIn(httpd_req_t *req) {
    auto server = static_cast<Server*>(req->user_ctx);

    std::unique_ptr<char[]> data = std::make_unique<char[]>(256);
    int ret = httpd_req_recv(req, data.get(), 256);
    if (ret < 0) {
        ESP_LOGW("server", "Reading POST data failed: %d.", ret);
        // Docs say we have to return an error if this happens.
        return ESP_FAIL;
    }
    if (ret == 256) {
        // We probably cut off. We aren't actually supporting this - just give up here.
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too large");
        return ESP_OK;
    }

    std::unique_ptr<char[]> email = std::make_unique<char[]>(120);
    std::unique_ptr<char[]> password = std::make_unique<char[]>(101);

    esp_err_t err = httpd_query_key_value(data.get(), "username", email.get(), 120);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad email");
        return ESP_OK;
    }

    err = httpd_query_key_value(data.get(), "password", password.get(), 100);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad password");
        return ESP_OK;
    }

    std::string emailStr = http::urlDecode(email.get());
    std::string passwordStr = http::urlDecode(password.get());

    server->client->setLogin(emailStr, passwordStr);

    // If we got this far, we're actually doing something.
    // Stash this away so we can respond asynchronously.
    int fd = httpd_req_to_sockfd(req);

    server->client->authenticate([=](bool successful) {
        struct context {
            Server *server;
            int fd;
            bool successful;
            std::string email;
            std::string password;
        };
        auto *ctx = new context {
            .server = server,
            .fd = fd,
            .successful = successful,
            .email = emailStr,
            .password = passwordStr,
        };
        httpd_queue_work(server->httpd, [](void *arg) {
            auto *ctx = static_cast<context*>(arg);
            std::string json = R"({"success": )"s + (ctx->successful ? "true"s : "false"s) + "}"s;
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(json.length()) + "\r\n\r\n";
            ctx->server->sendSocketData(ctx->fd, response);
            ctx->server->sendSocketData(ctx->fd, json);
            ctx->server->complete(ctx->email, ctx->password);
        }, ctx);
    });

    return ESP_OK;
}

void Server::complete(const std::string &email, const std::string &password) {
    // We're probably being called from the thing we're about to tear down, so kick ourselves to a new task for this...
    config->storeInitialConfig(email, password);
    if (completionCallback) {
        completionCallback(true);
    }
}

void Server::setCompletionCallback(const std::function<void(bool)> &fn) {
    completionCallback = fn;
}

}