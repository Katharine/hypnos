#include "scheduler.h"

#include <Arduino.h>

namespace scheduler {

void scheduler::schedule_repeating_task(unsigned long interval_ms, const scheduler_task& fn) {
    queue.push(internal::task{
        .repeat_interval_ms = interval_ms,
        .fn = fn,
        .next_instance = delayed_by_ms(get_absolute_time(), interval_ms),
    });
}

void scheduler::tick() {
    const internal::task& next = queue.top();
    if (to_us_since_boot(get_absolute_time()) < to_us_since_boot(next.next_instance)) {
        return;
    }
    next.fn();
    internal::task new_task = {
        .repeat_interval_ms = next.repeat_interval_ms,
        .fn = std::move(next.fn),
        .next_instance = delayed_by_ms(next.next_instance, next.repeat_interval_ms),
    };
    queue.pop();
    queue.push(new_task);
}

}