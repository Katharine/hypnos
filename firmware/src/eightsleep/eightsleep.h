#ifndef eightsleep_h
#define eightsleep_h
#define DEBUG_HTTPCLIENT(fmt, ...) Serial.printf(fmt, ## __VA_ARGS__ )
#include <string>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include "internal/apiclient.h"
#include "userauth.h"
#include "alarm.h"

namespace eightsleep {

/**
 * eightsleep provides a mechanism to interact with the Eight Sleep API.
 */
class eightsleep {
private:
    std::string username;
    std::string password;

    internal::api_client client;

public:
    eightsleep() {}
    eightsleep(const std::string& username, const std::string& password) : username(username), password(password) {}

    void set_login(const std::string& username, const std::string& password);

    /// Authenticates with both the legacy and modern Eight Sleep APIs. Requires for any subsequent call.
    bool authenticate();

    /// Fetches a vector of alarms that the user has set. May fail if the user has set too many alarms.
    rd::expected<std::vector<alarm>, std::string> get_alarms();

    /// Stops any active alarms. 
    bool stop_alarms();
};
}

#endif
