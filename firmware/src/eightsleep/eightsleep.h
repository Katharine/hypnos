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

class eightsleep {
private:
    std::string username;
    std::string password;

    internal::api_client client;

public:
    eightsleep(const std::string& username, const std::string& password) : username(username), password(password) {
    }

    bool authenticate();
    rd::expected<std::vector<alarm>, std::string> get_alarms();

private:
    std::optional<std::pair<std::string, std::string>> retrieveToken(WiFiClient& client);
};
}

#endif
