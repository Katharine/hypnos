#include <string>
#include <sstream>
#include <expected.hpp>
#include <vector>
#include <esp_log.h>

#include "eightsleep.h"
#include "alarm.h"
#include "internal/apiclient.h"

namespace eightsleep {

static Weekdays parseRepeatDays(JsonObject repeat);

void Client::authenticate(const std::function<void(bool)>& cb) {
    ESP_LOGI(TAG, "Making initial auth request...");
    client.makeUnauthedRequest({
        .url = "https://client-api.8slp.net/v1/login",
        .method = HTTP_METHOD_POST,
        .json_doc_size = 300,
        .payload = internal::ApiClient::makePostData({{"email", username}, {"password", password}}),
        .callback = [=, this](const rd::expected<DynamicJsonDocument, std::string>& loginResult) {
            if (!loginResult) {
                ESP_LOGE(TAG, "Auth request failed :(");
                cb(false);
                return;
            }
            ESP_LOGI(TAG, "Updating legacy userId/token");
            if (!loginResult->containsKey("session")) {
                ESP_LOGE(TAG, "Auth result doesn't contain session object");
                cb(false);
                return;
            }
            JsonObjectConst obj = loginResult.value()["session"];
            if (!obj.containsKey("userId") || !obj.containsKey("token")) {
                ESP_LOGE(TAG, "userId and/or token are missing from the returned session object.");
                cb(false);
                return;
            }
            client.auth.user_id = loginResult.value()["session"]["userId"].as<std::string>();
            client.auth.legacy_token = loginResult.value()["session"]["token"].as<std::string>();
            ESP_LOGI(TAG, "User ID: %s", client.auth.user_id->c_str());
            ESP_LOGI(TAG, "Legacy token: %s", client.auth.legacy_token->c_str());

            ESP_LOGI(TAG, "Trading for OAuth token...");
            client.makeLegacyRequest({
                .url = "https://client-api.8slp.net/v1/users/oauth-token",
                .method = HTTP_METHOD_POST,
                .json_doc_size = 1536,
                .payload = internal::ApiClient::makePostData(
                    {{"client_id",     internal::EIGHTSLEEP_CLIENT_ID},
                     {"client_secret", internal::EIGHTSLEEP_CLIENT_SECRET}}),
                .callback = [=, this](const rd::expected<DynamicJsonDocument, std::string>& exchangeResult) {
                    if (!exchangeResult) {
                        ESP_LOGI(TAG, "OAuth exchange failed");
                        cb(false);
                        return;
                    }
                    if (!exchangeResult->containsKey("access_token")) {
                        ESP_LOGI(TAG, "OAuth exchange response has no access token");
                        cb(false);
                        return;
                    }
                    client.auth.oauth_token = exchangeResult.value()["access_token"].as<std::string>();
                    ESP_LOGI(TAG, "Authentication complete.");
                    cb(true);
                },
           });
       },
    });
}


void Client::setLogin(const std::string& u, const std::string& p) {
    this->username = u;
    this->password = p;
}

void Client::getAlarms(const std::function<void(rd::expected<std::vector<Alarm>, std::string>)>& cb) {
    if (!client.auth.user_id) {
        cb(rd::unexpected("user is not logged in"));
        return;
    }
    client.makeOauthRequest({
        .url = "https://app-api.8slp.net/v1/users/" + *client.auth.user_id + "/alarms",
        .method = HTTP_METHOD_GET,
        .json_doc_size = 8192,
        .callback = [=](rd::expected<DynamicJsonDocument, std::string> alarmResult) {
            if (!alarmResult) {
                cb(rd::unexpected("fetching alarms failed: " + alarmResult.error()));
                return;
            }
            JsonArray json_alarms = alarmResult.value()["alarms"];
            std::vector<Alarm> alarms;
            alarms.reserve(json_alarms.size());

            for (JsonObject ja : json_alarms) {
                Alarm aa = Alarm{
                    .id = ja["id"],
                    .enabled = ja["enabled"],
                    .time = ja["time"],
                    .repeatDays = parseRepeatDays(ja["repeat"]),
                    .vibration = ja["vibration"]["enabled"] ? ja["vibration"]["powerLevel"] : 0,
                    .thermal = ja["thermal"]["enabled"] ? ja["thermal"]["level"] : 0,
                    .nextTime = ja["nextTimestamp"],
                    .snoozing = ja["snoozing"],
                };
                alarms.push_back(aa);
            }

            cb(alarms);
        },
    });
}

void Client::stopAlarms(std::function<void(bool)> cb) {
    if (!client.auth.user_id || !client.auth.oauth_token) {
        cb(false);
        return;
    }
    client.makeOauthRequest({
        .url = "https://app-api.8slp.net/v1/users/" + *client.auth.user_id + "/alarms/active/stop",
        .method = HTTP_METHOD_PUT,
        .callback = [=](rd::expected<DynamicJsonDocument, std::string> result) {
            if (!result) {
                ESP_LOGW(TAG, "stop alarm request failed: %s", result.error().c_str());
                cb(false);
                return;
            }
            cb(true);
        },
    });
}

void Client::getDeviceId(const std::function<void(rd::expected<std::string, std::string>)> &cb) {
    ESP_LOGI(TAG, "Fetching device ID");
    client.makeLegacyRequest({
        .url = "https://client-api.8slp.net/v1/users/me",
        .method = HTTP_METHOD_GET,
        .json_doc_size = 2048,
        .callback = [=, this](rd::expected<DynamicJsonDocument, std::string> result) {
            if (!result) {
                ESP_LOGE(TAG, "Fetching device ID failed: %s", result.error().c_str());
                cb(rd::unexpected(result.error()));
                return;
            }
            JsonObjectConst obj = result.value()["user"]["currentDevice"];
            if (obj.containsKey("id") && !obj["id"].isNull()) {
                ESP_LOGI(TAG, "Device ID is %s", obj["id"].as<const char*>());
                deviceId = obj["id"].as<std::string>();
                bedSide = obj["side"].as<std::string>();
                cb(obj["id"].as<std::string>());
                return;
            } else {
                ESP_LOGE(TAG, "Fetching device ID failed.");
                ESP_LOGE(TAG, "%s", result->as<std::string>().c_str());
            }
        },
    });
}

void Client::getBedStatus(const std::function<void(rd::expected<Bed, std::string>)> &cb) {
    auto doWork = [=, this](rd::expected<std::string, std::string> input) {
        if (!input) {
            ESP_LOGE(TAG, "Can't fetch device info: %s.", input.error().c_str());
            cb(rd::unexpected(input.error()));
            return;
        }
        ESP_LOGI(TAG, "Fetching device info.");
        client.makeLegacyRequest({
            .url = "https://client-api.8slp.net/v1/devices/" + deviceId,
            .method = HTTP_METHOD_GET,
            .json_doc_size = 4096, // TODO: presumably we could get this down a lot with filters.
            .callback = [=, this](rd::expected<DynamicJsonDocument, std::string> result) {
                if (!result) {
                    cb(rd::unexpected(result.error()));
                    return;
                }
                // I have no idea why this is referred to as "kelvin" all the time. That's not the actual unit.
                // (in fact, these are just in mysterious "levels").
                JsonObject kelvin = result.value()["result"][bedSide + "Kelvin"];
                if (kelvin.isNull()) {
                    cb(rd::unexpected("Request made, but no data returned."));
                    return;
                }
                Bed bed = {
                    .currentTemp = kelvin["level"].as<int>(),
                    .targetTemp = kelvin["currentTargetLevel"].as<int>(),
                    .active = kelvin["active"].as<bool>(),
                };
                cb(bed);
            },
        });
    };
    if (!deviceId.empty()) {
        doWork(deviceId);
    } else {
        ESP_LOGI(TAG, "Fetching device ID before trying to fetch device info...");
        getDeviceId(std::move(doWork));
    }
}

static Weekdays parseRepeatDays(JsonObject repeat) {
    if (!repeat["enabled"]) {
        return Weekdays::None;
    }
    JsonObject repeat_days = repeat["Weekdays"];
    Weekdays days = Weekdays::None;
    if (repeat_days["monday"]) {
        days |= Weekdays::Monday;
    }
    if (repeat_days["tuesday"]) {
        days |= Weekdays::Tuesday;
    }
    if (repeat_days["wednesday"]) {
        days |= Weekdays::Wednesday;
    }
    if (repeat_days["thursday"]) {
        days |= Weekdays::Thursday;
    }
    if (repeat_days["friday"]) {
        days |= Weekdays::Friday;
    }
    if (repeat_days["saturday"]) {
        days |= Weekdays::Saturday;
    }
    if (repeat_days["sunday"]) {
        days |= Weekdays::Sunday;
    }
    return days;
}

}