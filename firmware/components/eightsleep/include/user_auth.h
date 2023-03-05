#ifndef EIGHTSLEEP_INCLUDE_USER_AUTH_H
#define EIGHTSLEEP_INCLUDE_USER_AUTH_H

#include <optional>
#include <string>

namespace eightsleep {

struct UserAuth {
    std::optional<std::string> user_id;
    std::optional<std::string> legacy_token;
    std::optional<std::string> oauth_token;
};

}

#endif