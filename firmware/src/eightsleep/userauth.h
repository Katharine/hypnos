#ifndef eightsleep_userauth_h
#define eightsleep_userauth_h

#include <string>
#include <optional>

namespace eightsleep {

struct user_auth {
    std::optional<std::string> user_id;
    std::optional<std::string> legacy_token;
    std::optional<std::string> oauth_token;
};

}

#endif
