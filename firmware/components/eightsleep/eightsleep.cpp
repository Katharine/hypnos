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
                    if (cb) {
                        cb(true);
                    }
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

void Client::hasActiveAlarm(const std::function<void(rd::expected<bool, std::string>)> &cb) {
    if (!client.auth.user_id) {
        cb(rd::unexpected("user is not logged in"));
        return;
    }
    client.makeOauthRequest({
        .url = "https://app-api.8slp.net/v1/users/" + *client.auth.user_id + "/alarms/active",
        .method = HTTP_METHOD_GET,
        .json_doc_size = 4096,
        .callback = [=, this](rd::expected<DynamicJsonDocument, std::string> alarmResult) {
            if (!alarmResult) {
                cb(rd::unexpected("fetching alarms failed: " + alarmResult.error()));
                return;
            }
            // If there's an "alarm" key (singular), then an alarm is currently going off.
            // The "alarms" key (plural) is always present but is irrelevant and always empty.
            cb(alarmResult->containsKey("alarm"));
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
                cb(parseBedResult(result.value()));
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

rd::expected<Bed, std::string> Client::parseBedResult(const DynamicJsonDocument& doc) {
    // I have no idea why this is referred to as "kelvin" all the time. That's not the actual unit.
    // (in fact, these are just in mysterious "levels").
    JsonVariantConst kelvin;
    if (doc.containsKey("result")) {
        kelvin = doc["result"][bedSide + "Kelvin"];
    } else {
        kelvin = doc.as<JsonVariantConst>();
    }
    if (kelvin.isNull() || !kelvin.containsKey("level") || !kelvin.containsKey("currentTargetLevel") || !kelvin.containsKey("active")) {
        return rd::unexpected("Request made, but no data returned.");
    }
    return Bed{
        // The "level" in the kelvin block is not actually the current level.
        // We have to pull this out of the root object.
        // Some of our callers won't have this present - in that case, this will be
        // zero. Callers are expected to know when this works.
        .currentTemp = doc["result"][bedSide + "HeatingLevel"].as<int>(),
        .targetTemp = kelvin["currentTargetLevel"].as<int>(),
        .active = kelvin["active"].as<bool>(),
    };
}

void Client::setBedState(bool on, const std::function<void(rd::expected<Bed, std::string>)> &cb) {
    auto doWork = [=, this](rd::expected<std::string, std::string> input) {
        if (!input) {
            ESP_LOGE(TAG, "Can't fetch device info: %s.", input.error().c_str());
            cb(rd::unexpected(input.error()));
            return;
        }
        int duration = on ? 72000 : 0;
        ESP_LOGI(TAG, "Setting bed duration to %d", duration);
        client.makeLegacyRequest({
            .url = "https://client-api.8slp.net/v1/devices/" + deviceId + "/" + bedSide + "/duration/" + std::to_string(duration),
            .method = HTTP_METHOD_PUT,
            .json_doc_size = 768,
            .callback = [=, this](rd::expected<DynamicJsonDocument, std::string> result) {
                if (!result) {
                    cb(rd::unexpected(result.error()));
                    return;
                }
                cb(parseBedResult(result.value()));
            },
        });
    };
    if (!deviceId.empty()) {
        doWork(deviceId);
    } else {
        ESP_LOGI(TAG, "Fetching device ID before trying to set bed state...");
        getDeviceId(std::move(doWork));
    }
}

void Client::setTemp(int temp, const std::function<void(rd::expected<Bed, std::string>)> &cb) {
    auto doWork = [=, this](rd::expected<std::string, std::string> input) {
        if (!input) {
            ESP_LOGE(TAG, "Can't fetch device info: %s.", input.error().c_str());
            cb(rd::unexpected(input.error()));
            return;
        }
        ESP_LOGI(TAG, "Setting bed temp to %d", temp);
        client.makeLegacyRequest({
            .url = "https://client-api.8slp.net/v1/devices/" + deviceId + "/" + bedSide + "/level/" + std::to_string(temp),
            .method = HTTP_METHOD_PUT,
            .json_doc_size = 768,
            .callback = [=, this](rd::expected<DynamicJsonDocument, std::string> result) {
                if (!result) {
                    cb(rd::unexpected(result.error()));
                    return;
                }
                cb(parseBedResult(result.value()));
            },
        });
    };
    if (!deviceId.empty()) {
        doWork(deviceId);
    } else {
        ESP_LOGI(TAG, "Fetching device ID before trying to set bed temperature...");
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