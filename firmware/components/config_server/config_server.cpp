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

Server::Server(const std::shared_ptr<wifi::WiFi>& wifi) : wifi(wifi) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&httpd, &config));
    config.uri_match_fn = httpd_uri_match_wildcard;

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
}

Server::~Server() {
    httpd_stop(httpd);
}

int Server::serveIndex(httpd_req_t *req) {
    auto f = cmrc::web::get_filesystem().open("index.html");
    httpd_resp_send(req, f.begin(), f.end() - f.begin());
    return ESP_OK;
}

int Server::serveScan(httpd_req_t *req) {
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
    }
    err = httpd_query_key_value(data.data(), "psk", psk.data(), psk.size() - 1);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad PSK");
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
    });
    server->wifi->joinNetwork(ssid.data(), psk.data());

    return ESP_OK;
}

}