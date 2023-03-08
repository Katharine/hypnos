#include "http_cxx.hpp"

#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <utility>
#include <vector>
#include <esp_log.h>
#include <mbedtls/error.h>
#include <esp_tls.h>
#include <sstream>

namespace http {

Client::Client() : Client("https://example.com") {

}

Client::Client(const std::string &url) {
    esp_http_client_config_t config = {
        .url = url.c_str(),
        .event_handler = &Client::handleEvent,
        .buffer_size_tx = 2048,
        .user_data = this,
        .is_async = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    handle = esp_http_client_init(&config);
}

Client::~Client() {
    esp_http_client_cleanup(handle);
}

esp_err_t Client::handleEvent(esp_http_client_event_t *evt) {
    auto *client = static_cast<Client*>(evt->user_data);
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI("http", "Data received: %d bytes", evt->data_len);
            if (client->response.capacity() == 0) {
                int64_t length = esp_http_client_get_content_length(evt->client);
                if (length > 0) {
                    ESP_LOGI("http", "Allocating %d bytes for data", (int)length);
                    client->response.reserve(length);
                }
            }
            for (int i = 0; i < evt->data_len; ++i) {
                client->response.push_back(reinterpret_cast<char*>(evt->data)[i]);
            }
            break;
        case HTTP_EVENT_ON_HEADER:
            client->incomingHeaders[evt->header_key] = evt->header_value;
            break;
        case HTTP_EVENT_ON_FINISH: {
            ESP_LOGI("http", "Finished. Preparing callback...");
            Response resp = {
                .body = std::string(client->response.data(), client->response.size()),
                .status_code = esp_http_client_get_status_code(evt->client),
                .headers = client->incomingHeaders,
            };
            client->body = "";
            std::vector<char>().swap(client->response);
            client->incomingHeaders.clear();
            if (client->callback) {
                ESP_LOGI("http", "Calling back.");
                client->callback(true, resp);
                client->callback = nullptr;
            }
            client->busy = false;
            break;
        }
        case HTTP_EVENT_DISCONNECTED: {
            if (!client->busy) {
                ESP_LOGD("http", "HTTP_EVENT_DISCONNECTED on already-finished client, ignoring.");
                break;
            }
            ESP_LOGI("http", "HTTP_EVENT_DISCONNECTED");
        }
        case HTTP_EVENT_ERROR:
            ESP_LOGI("http", "Failed. Preparing callback...");
            // handle error
            client->body = "";
            std::vector<char>().swap(client->response);
            client->incomingHeaders.clear();
            if (client->callback) {
                ESP_LOGI("http", "Calling back.");
                client->callback(false, Response{});
                client->callback = nullptr;
            }
            client->busy = false;
            break;
        default:
            // we don't care.
            break;
    }
    return ESP_OK;
}

void Client::request(esp_http_client_method_t method, const std::string &url, const std::string &temp_body, Callback cb) {
    if (busy) {
        ESP_LOGE("http", "Received a request while we're already busy.");
        return;
    }
    busy = true;
    ESP_ERROR_CHECK(esp_http_client_set_method(handle, method));
    if (!url.empty()) {
        ESP_ERROR_CHECK(esp_http_client_set_url(handle, url.c_str()));
    }
    if (!userAgent.empty()) {
        ESP_ERROR_CHECK(esp_http_client_set_header(handle, "User-Agent", userAgent.c_str()));
    }
    for (const auto& [name, value] : outgoingHeaders) {
        ESP_ERROR_CHECK(esp_http_client_set_header(handle, name.c_str(), value.c_str()));
    }
    if (!temp_body.empty()) {
        body = temp_body; // We need to keep a copy of this around.
    }
    ESP_ERROR_CHECK(esp_http_client_set_post_field(handle, body.c_str(), static_cast<int>(body.length())));
    preparedRequest(std::move(cb));
}

void Client::preparedRequest(Callback cb) {
    callback = std::move(cb);
    int attempts = 0;
    while (true) {
        ++attempts;
        esp_err_t err = esp_http_client_perform(handle);
        if (!busy) {
            return;
        }
        if (err != ESP_ERR_HTTP_EAGAIN) {
            ESP_LOGI("http", "client_perform_attempts: %d", attempts);
            if (err != ESP_OK) {
                if (callback) {
                    callback(false, Response{});
                }
            }
            break;
        }
    }
}

void Client::get(const std::string &url, Callback cb) {
    request(HTTP_METHOD_GET, url, "", std::move(cb));
}

void Client::post(const std::string &url, const std::string &temp_body, Callback cb) {
    request(HTTP_METHOD_POST, url, temp_body, std::move(cb));
}

void Client::put(const std::string &url, const std::string &temp_body, Callback cb) {
    request(HTTP_METHOD_PUT, url, temp_body, std::move(cb));
}

void Client::setUserAgent(const std::string &agent) {
    userAgent = agent;
}

void Client::setHeader(const std::string &name, const std::string &value) {
    outgoingHeaders[name] = value;
}


std::string urlDecode(const std::string& thing) {
    std::ostringstream oss;
    for (size_t i = 0, size = thing.length(); i < size; ++i) {
        if(thing[i] != '%') {
            oss << thing[i];
            continue;
        }
        char digits[3] = {thing[i+1], thing[i+2], 0};
        i += 2;
        char ch = static_cast<char>(std::strtol(digits, nullptr, 16));
        oss << ch;
    }
    return oss.str();
}

}