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

namespace eightsleep {


class Client {
private:
    std::string username;
    std::string password;

    internal::ApiClient client = internal::ApiClient();

public:
    Client() = default;
    Client(std::string username, std::string password) : username(std::move(username)), password(std::move(password)) {}

    void setLogin(const std::string& u, const std::string& p);

    /// Authenticates with both the legacy and modern Eight Sleep APIs. Requires for any subsequent call.
    void authenticate(const std::function<void(bool)>& cb);

    /// Fetches a vector of alarms that the user has set. May fail if the user has set too many alarms.
    void getAlarms(const std::function<void(rd::expected<std::vector<Alarm>, std::string>)>& alarmResult);

    /// Stops any active alarms.
    void stopAlarms(std::function<void(bool)> cb);
};

}

#endif
