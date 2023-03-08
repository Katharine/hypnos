#include <string>
#include <iomanip>
#include <sstream>
#include <expected.hpp>
#include <esp_log.h>
#include <esp_timer.h>

#include <ArduinoJson.h>

#include "apiclient.h"

using namespace std::string_literals;

namespace eightsleep::internal {

void ApiClient::makeUnauthedRequest(const RequestParams& params) {
    std::shared_ptr<http::Client> c = std::make_shared<http::Client>(params.url);
    c->setUserAgent("okhttp/3.6.0");
    if (params.method != HTTP_METHOD_GET) {
        c->setHeader("Content-Type", params.content_type);
    }
    requestWithRetries(c, params.method, params.payload, params.retry_limit, [=](bool success, const http::Response& response) {
        if (!success) {
            params.callback(rd::unexpected("making request failed"));
            return;
        }
        DynamicJsonDocument doc(params.json_doc_size);
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            params.callback(rd::unexpected("JSON decode failed: "s + error.c_str()));
            return;
        }
        params.callback(std::move(doc));
    });
};

void ApiClient::makeLegacyRequest(const RequestParams& params) {
    if (!auth.legacy_token) {
        params.callback(rd::unexpected("not logged in!"));
    }
    std::shared_ptr<http::Client> c = std::make_shared<http::Client>(params.url);
    c->setUserAgent("okhttp/3.6.0");
    c->setHeader("Session-Token", *auth.legacy_token);
    if (params.method != HTTP_METHOD_GET) {
       c->setHeader("Content-Type", params.content_type);
    }
    requestWithRetries(c, params.method, params.payload, params.retry_limit, [=](bool success, const http::Response& response) {
        if (!success) {
            params.callback(rd::unexpected("making request failed"));
            return;
        }
        DynamicJsonDocument doc(params.json_doc_size);
        DeserializationError error = deserializeJson(doc, response.body);
        if (error) {
            params.callback(rd::unexpected("JSON decode failed: "s + error.c_str()));
            return;
        }
        params.callback(std::move(doc));
    });
}

void ApiClient::makeOauthRequest(const RequestParams& params) {
    if (!auth.oauth_token) {
        params.callback(rd::unexpected("no oauth token available!"));
    }
    std::shared_ptr<http::Client> c = std::make_shared<http::Client>(params.url);
    c->setUserAgent("okhttp/3.6.0");
    c->setHeader("Authorization", "Bearer " + *auth.oauth_token);
    if (params.method != HTTP_METHOD_GET) {
        c->setHeader("Content-Type", params.content_type);
    }
    requestWithRetries(c, params.method, params.payload, params.retry_limit, [=](bool success, const http::Response& response) {
        if (!success) {
            params.callback(rd::unexpected("making request failed"));
            return;
        }
        DynamicJsonDocument doc(params.json_doc_size);
        // No response at all is a success as far as we're concerned.
        if (!response.body.empty()) {
            DeserializationError error = deserializeJson(doc, response.body);
            if (error) {
                params.callback(rd::unexpected("JSON decode failed: "s + error.c_str()));
                return;
            }
        }
        params.callback(std::move(doc));
    });
}

std::string ApiClient::urlEncode(const std::string& thing) {
    std::ostringstream oss;
    for (auto& ch : thing) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            oss << ch;
        } else {
            oss << "%" << std::hex << std::setw(2) << std::setfill('0') << (int)ch;
        }
    }
    return oss.str();
}

std::string ApiClient::urlDecode(const std::string& thing) {
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

std::string ApiClient::makePostData(const std::map<std::string, std::string>& data) {
    std::ostringstream oss;
    bool first = true;
    for (auto const& [key, value] : data) {
        if (!first) {
            oss << "&";
        } else {
            first = false;
        }
        oss << urlEncode(key) << "=" << urlEncode(value);
    }
    return oss.str();
}

void ApiClient::requestWithRetries(const std::shared_ptr<http::Client>& c, esp_http_client_method_t method, const std::string &payload, int retry_count,
                                  const http::Callback& callback) {
    ESP_LOGI("apiclient", "Making request.");
    c->request(method, "", payload, [=](bool success, const http::Response& response) {
        if (success || retry_count == 0) {
            callback(success, response);
            return;
        } else {
            ESP_LOGI("apiclient", "Retrying... %d attempts remain", retry_count);
            struct context {
                ApiClient *client;
                std::shared_ptr<http::Client> c;
                esp_http_client_method_t method;
                std::string payload;
                int retry_count;
                http::Callback callback;
                esp_timer_handle_t timer;
            };
            auto *ctx = new context {
                .client = this,
                .c = c,
                .method = method,
                .payload = payload,
                .retry_count = retry_count,
                .callback = callback,
            };
            esp_timer_create_args_t args = {
                .callback = [](void *arg) {
                    ESP_LOGI("apiclient", "Retrying triggered");
                    auto *ctx = static_cast<context*>(arg);
                    ctx->client->requestWithRetries(ctx->c, ctx->method, ctx->payload, ctx->retry_count - 1, ctx->callback);
                    esp_timer_delete(ctx->timer);
                    delete ctx;
                },
                .arg = ctx,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "client-retry",
            };
            esp_timer_create(&args, &ctx->timer);
            esp_timer_start_once(ctx->timer, 5000000);
        }
    });
}

}