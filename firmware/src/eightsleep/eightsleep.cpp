#include <string>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <expected.hpp>
#include <vector>

#include "eightsleep.h"
#include "alarm.h"
#include "internal/apiclient.h"

namespace eightsleep {

static weekdays parse_repeat_days(JsonObject repeat);

bool eightsleep::authenticate() {
    auto login_result = client.make_unauthed_request({
        .url = "https://client-api.8slp.net/v1/login",
        .method = "POST",
        .json_doc_size = 300,
        .payload = internal::api_client::make_post_data({{"email", username}, {"password", password}}),
    });
    if (!login_result) {
        return false;
    }
    client.auth.user_id = login_result.value()["session"]["userId"].as<std::string>();
    client.auth.legacy_token = login_result.value()["session"]["token"].as<std::string>();

    auto exchange_result = client.make_legacy_request({
        .url = "https://client-api.8slp.net/v1/users/oauth-token",
        .method = "POST",
        .json_doc_size = 1536,
        .payload = internal::api_client::make_post_data({{"client_id", internal::EIGHTSLEEP_CLIENT_ID}, {"client_secret", internal::EIGHTSLEEP_CLIENT_SECRET}})
    });
    if (!exchange_result) {
        return false;
    }
    client.auth.oauth_token = exchange_result.value()["access_token"].as<std::string>();
    return true;
}

rd::expected<std::vector<alarm>, std::string> eightsleep::get_alarms() {
    if (!client.auth.user_id) {
        return rd::unexpected("user is not logged in");
    }
    auto alarms_result = client.make_oauth_request({
        .url = "https://app-api.8slp.net/v1/users/" + *client.auth.user_id + "/alarms",
        .method = "GET",
        .json_doc_size = 8192,
    });
    if (!alarms_result) {
        return rd::unexpected("fetching alarms failed: " + alarms_result.error());
    }
    JsonArray json_alarms = alarms_result.value()["alarms"];
    std::vector<alarm> alarms;
    alarms.reserve(json_alarms.size());

    for (JsonObject ja : json_alarms) {
        alarm aa = alarm{
            .id = ja["id"],
            .enabled = ja["enabled"],
            .time = ja["time"],
            .repeat_days = parse_repeat_days(ja["repeat"]),
            .vibration = ja["vibration"]["enabled"] ? ja["vibration"]["powerLevel"] : 0,
            .thermal = ja["thermal"]["enabled"] ? ja["thermal"]["level"] : 0,
            .next_time = ja["nextTimestamp"],
            .snoozing = ja["snoozing"],
        };
        alarms.push_back(aa);
    }

    return alarms;
}

static weekdays parse_repeat_days(JsonObject repeat) {
    if (!repeat["enabled"]) {
        return weekdays::none;
    }
    JsonObject repeat_days = repeat["weekDays"];
    weekdays days;
    if (repeat_days["monday"]) {
        days |= weekdays::monday;
    }
    if (repeat_days["tuesday"]) {
        days |= weekdays::tuesday;
    }
    if (repeat_days["wednesday"]) {
        days |= weekdays::wednesday;
    }
    if (repeat_days["thursday"]) {
        days |= weekdays::thursday;
    }
    if (repeat_days["friday"]) {
        days |= weekdays::friday;
    }
    if (repeat_days["saturday"]) {
        days |= weekdays::saturday;
    }
    if (repeat_days["sunday"]) {
        days |= weekdays::sunday;
    }
    return days;
}

}