#include <string>
#include <iomanip>
#include <sstream>
#include <expected.hpp>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "apiclient.h"

namespace eightsleep {
namespace internal {

api_client::api_client() {
    client.setUserAgent("okhttp/3.6.0");
}

rd::expected<DynamicJsonDocument, std::string> api_client::make_unauthed_request(const request_params& params) {
    if (!client.begin(params.url.c_str())) {
        return rd::unexpected("failed to begin request");
    }
    client.setInsecure(); // Until we include a proper cert bundle.
    if (params.method != "GET") {
        client.addHeader("Content-Type", params.content_type.c_str());
    }
    int result = request_with_retries(params.method.c_str(), params.payload.c_str(), params.retry_limit);
    if (result < 0) {
        client.end();
        return rd::unexpected("making request failed.");
    }
    DynamicJsonDocument doc(params.json_doc_size);
    DeserializationError error = deserializeJson(doc, client.getString());
    if (error) {
        client.end();
        return rd::unexpected("JSON decode failed: " + std::string(error.c_str()));
    }
    client.end();
    return doc;
};

rd::expected<DynamicJsonDocument, std::string> api_client::make_legacy_request(const request_params& params) {
    if (!auth.legacy_token) {
        return rd::unexpected("not logged in!");
    }
    if (!client.begin(params.url.c_str())) {
        return rd::unexpected("failed to begin request");
    }
    client.setInsecure(); // Until we include a proper cert bundle.
    client.addHeader("Session-Token", auth.legacy_token->c_str());
    if (params.method != "GET") {
        client.addHeader("Content-Type", params.content_type.c_str());
    }
    int result = request_with_retries(params.method.c_str(), params.payload.c_str(), params.retry_limit);
    if (result < 0) {
        client.end();
        return rd::unexpected("making request failed.");
    }
    DynamicJsonDocument doc(params.json_doc_size);
    DeserializationError error = deserializeJson(doc, client.getString());
    if (error) {
        client.end();
        return rd::unexpected("JSON decode failed: " + std::string(error.c_str()));
    }
    client.end();
    return doc;
}

rd::expected<DynamicJsonDocument, std::string> api_client::make_oauth_request(const request_params& params) {
    if (!auth.oauth_token) {
        return rd::unexpected("no oauth token available!");
    }
    if (!client.begin(params.url.c_str())) {
        return rd::unexpected("failed to begin request");
    }
    client.setInsecure(); // Until we include a proper cert bundle.
    client.addHeader("Authorization", String(("Bearer " + *auth.oauth_token).c_str()));
    if (params.method != "GET") {
        client.addHeader("Content-Type", params.content_type.c_str());
    }
    int result = request_with_retries(params.method.c_str(), params.payload.c_str(), params.retry_limit);
    if (result < 0) {
        client.end();
        return rd::unexpected("making request failed.");
    }
    DynamicJsonDocument doc(params.json_doc_size);
    DeserializationError error = deserializeJson(doc, client.getString());
    if (error) {
        client.end();
        return rd::unexpected("JSON decode failed: " + std::string(error.c_str()));
    }
    client.end();
    return doc;
}

std::string api_client::url_encode(const std::string& thing) {
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

std::string api_client::make_post_data(const std::map<std::string, std::string>& data) {
    std::ostringstream oss;
    bool first = true;
    for (auto const& [key, value] : data) {
        if (!first) {
            oss << "&";
        } else {
            first = false;
        }
        oss << url_encode(key) << "=" << url_encode(value);
    }
    return oss.str();
}


int api_client::request_with_retries(const std::string& method, const std::string& payload, int retry_count) {
    while (retry_count >= 0) {
        --retry_count;
        int ret = client.sendRequest(method.c_str(), payload.c_str());
        if (ret >= 0) {
            return ret;
        }
        Serial.println("Retrying...");
    }
    return -1;
}

}
}
