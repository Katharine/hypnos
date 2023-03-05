#ifndef eightsleep_internal_apiclient_h
#define eightsleep_internal_apiclient_h

#include <expected.hpp>
#include <ArduinoJson.h>
#include <map>
#include <http_cxx.hpp>

#include "user_auth.h"

namespace eightsleep::internal {

const std::string EIGHTSLEEP_CLIENT_ID = "0894c7f33bb94800a03f1f4df13a4f38";
const std::string EIGHTSLEEP_CLIENT_SECRET = "f0954a3ed5763ba3d06834c73731a32f15f168f47d4f164751275def86db0c76";

typedef std::function<void(rd::expected<DynamicJsonDocument, std::string>)> Callback;

struct RequestParams {
    const std::string& url;
    const esp_http_client_method_t method = HTTP_METHOD_GET;
    const size_t json_doc_size = 4096;
    const std::string& payload = "";
    const std::string& content_type = "application/x-www-form-urlencoded";
    const int retry_limit = 3;
    const Callback callback;
};

class ApiClient {
public:
    UserAuth auth;

    void makeUnauthedRequest(const RequestParams& params);
    void makeLegacyRequest(const RequestParams& params);
    void makeOauthRequest(const RequestParams& params);

    static std::string makePostData(const std::map<std::string, std::string>& data);
private:
    static std::string urlEncode(const std::string& thing);
    static std::string urlDecode(const std::string& thing);
    void requestWithRetries(const std::shared_ptr<http::Client>& c, esp_http_client_method_t method, const std::string& payload, int retry_count, const http::Callback& callback);
};

}

#endif