#ifndef eightsleep_internal_apiclient_h
#define eightsleep_internal_apiclient_h

#include <HTTPClient.h>
#include <expected.hpp>
#include <ArduinoJson.h>

#include "../userauth.h"

namespace eightsleep {
namespace internal {

const std::string EIGHTSLEEP_CLIENT_ID = "0894c7f33bb94800a03f1f4df13a4f38";
const std::string EIGHTSLEEP_CLIENT_SECRET = "f0954a3ed5763ba3d06834c73731a32f15f168f47d4f164751275def86db0c76";

struct request_params {
    const std::string& url;
    const std::string& method = "GET";
    const size_t json_doc_size = 4096;
    const std::string& payload = "";
    const std::string& content_type = "application/x-www-form-urlencoded";
    const int retry_limit = 3;
};

class api_client {
private:
    HTTPClient client;
public:
    user_auth auth;

    api_client();

    rd::expected<DynamicJsonDocument, std::string> make_unauthed_request(const request_params& params);
    rd::expected<DynamicJsonDocument, std::string> make_legacy_request(const request_params& params);
    rd::expected<DynamicJsonDocument, std::string> make_oauth_request(const request_params& params);

    static std::string make_post_data(const std::map<std::string, std::string>& data);
private:
    static std::string url_encode(const std::string& thing);
    int request_with_retries(const std::string& method, const std::string& payload, int retry_count);
};

}
}

#endif
