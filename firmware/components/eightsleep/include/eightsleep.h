#ifndef EIGHTSLEEP_EIGHTSLEEP_H
#define EIGHTSLEEP_EIGHTSLEEP_H

#include <string>
#include <utility>
#include <vector>
#include <expected.hpp>
#include <optional>
#include "alarm.h"
#include "../internal/apiclient.h"
#include "user_auth.h"
#include "bed.h"

namespace eightsleep {


class Client {
private:
    const char* TAG = "EightSleepClient";
    std::string username;
    std::string password;
    std::string deviceId;
    std::string bedSide;

    internal::ApiClient client = internal::ApiClient();

public:
    Client() = default;
    Client(std::string username, std::string password) : username(std::move(username)), password(std::move(password)) {}

    void setLogin(const std::string& u, const std::string& p);

    /// Authenticates with both the legacy and modern Eight Sleep APIs. Requires for any subsequent call.
    void authenticate(const std::function<void(bool)>& cb);

    /// Fetches a vector of alarms that the user has set. May fail if the user has set too many alarms.
    void getAlarms(const std::function<void(rd::expected<std::vector<Alarm>, std::string>)>& alarmResult);

    /// Fetches a vector of alarms that the user has set. May fail if the user has set too many alarms.
    void hasActiveAlarm(const std::function<void(rd::expected<bool, std::string>)>& cb);

    /// Stops any active alarms.
    void stopAlarms(std::function<void(bool)> cb);

    void getBedStatus(const std::function<void(rd::expected<Bed, std::string>)>& cb);

    /// Turn the bed on (true) or off (false). Actually turns the bed on for up to 20 hours.
    void setBedState(bool on, const std::function<void(rd::expected<Bed, std::string>)>& cb);

    /// Set the temperature of the bed (in weird Eight Sleep "level" units)
    void setTemp(int temp, const std::function<void(rd::expected<Bed, std::string>)>& cb);

private:
    void getDeviceId(const std::function<void(rd::expected<std::string, std::string>)>& cb);
    rd::expected<Bed, std::string> parseBedResult(const DynamicJsonDocument& doc);
};

}

#endif
