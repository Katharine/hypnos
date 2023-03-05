//
// Created by Katharine Berry on 3/4/23.
//

#ifndef EIGHTSLEEP_ALARM_H

#include <string>
#include <type_traits>

namespace eightsleep {

enum struct Weekdays : uint8_t {
    None = 0,
    Monday = 1 << 0,
    Tuesday = 1 << 1,
    Wednesday = 1 << 2,
    Thursday = 1 << 3,
    Friday = 1 << 4,
    Saturday = 1 << 5,
    Sunday = 1 << 6,
};

inline Weekdays operator| (Weekdays lhs, Weekdays rhs) {
    using T = std::underlying_type_t <Weekdays>;
    return static_cast<Weekdays>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline Weekdays operator|= (Weekdays& lhs, Weekdays rhs) {
    lhs = lhs | rhs;
    return lhs;
}

struct Alarm {
    std::string id;
    bool enabled;
    std::string time;
    Weekdays repeatDays;
    int vibration;
    int thermal;
    std::string nextTime;
    bool snoozing;
};

}

#define EIGHTSLEEP_ALARM_H

#endif //EIGHTSLEEP_ALARM_H
