#ifndef COMPONENT_HTTP_CXX_HTTP_CXX_H
#define COMPONENT_HTTP_CXX_HTTP_CXX_H

#include <esp_http_client.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace http {

struct Response {
    std::string body;
    int status_code;
    std::map<std::string, std::string> headers;
};

typedef std::function<void(bool,Response)> Callback;

class Client {
    static constexpr const char* TAG = "http_cxx";
public:
    Client();
    explicit Client(const std::string& url);
    ~Client();

    void setUserAgent(const std::string& agent);
    void request(esp_http_client_method_t  method, const std::string& url, const std::string& temp_body, Callback cb);
    void preparedRequest(Callback  cb);
    void get(const std::string& url, Callback callback);
    void setHeader(const std::string& name, const std::string& value);
    void post(const std::string& url, const std::string& body, Callback callback);
    void put(const std::string& url, const std::string& body, Callback callback);

private:
    esp_http_client_handle_t handle = nullptr;
    std::string body;
    std::string userAgent;
    std::vector<char> response = std::vector<char>();
    std::map<std::string, std::string> outgoingHeaders = std::map<std::string, std::string>();
    std::map<std::string, std::string> incomingHeaders = std::map<std::string, std::string>();
    Callback callback = nullptr;
    bool busy = false;

    static esp_err_t handleEvent(esp_http_client_event_t *evt);
};

std::string urlDecode(const std::string& thing);

}

#endif
