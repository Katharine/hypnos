#ifndef scheduler_scheduler_h
#define scheduler_scheduler_h

#include <functional>
#include <queue>
#include <Arduino.h>

namespace scheduler {

typedef std::function<void()> scheduler_task;

namespace internal {
    struct task {
        uint32_t repeat_interval_ms;
        std::function<void()> fn;
        absolute_time_t next_instance;
    };

    struct task_time_greater {
        bool operator()(const task& lhs, const task& rhs) const {
            return to_us_since_boot(lhs.next_instance) > to_us_since_boot(rhs.next_instance);
        }
    };

}

class scheduler {
    std::priority_queue<internal::task, std::vector<internal::task>, internal::task_time_greater> queue;

public:
    void schedule_repeating_task(unsigned long interval_ms, const scheduler_task& fn);
    void tick();
};

}

#endif
