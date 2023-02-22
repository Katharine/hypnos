#ifndef eightsleep_alarm_h
#define eightsleep_alarm_h

#include <string>

namespace eightsleep {

enum struct weekdays : uint8_t {
    none = 0,
    monday = 1 << 0,
    tuesday = 1 << 1,
    wednesday = 1 << 2,
    thursday = 1 << 3,
    friday = 1 << 4,
    saturday = 1 << 5,
    sunday = 1 << 6,
};

inline weekdays operator| (weekdays lhs, weekdays rhs) {
    using T = std::underlying_type_t <weekdays>;
    return static_cast<weekdays>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline weekdays operator|= (weekdays& lhs, weekdays rhs) {
    lhs = lhs | rhs;
    return lhs;
}

struct alarm {
    std::string id;
    bool enabled;
    std::string time;
    weekdays repeat_days;
    int vibration;
    int thermal;
    std::string next_time;
    bool snoozing;
};

}

#endif