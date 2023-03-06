//
// Created by Katharine Berry on 3/5/23.
//

#ifndef APPS_MAIN_STATE_H
#define APPS_MAIN_STATE_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <functional>
#include <memory>
#include <expected.hpp>

#include <eightsleep.h>

namespace apps::main {

enum struct Units {
    EightSleep,
    Celsius,
    Fahrenheit,
};

struct State {
    int bedTargetTemp = 0;
    int localTargetTemp = 0;
    int bedActualTemp = 0;
    bool bedState = false;
    bool requestedState = false;
    bool apiReachable = false;
    bool valid = false;
    Units units = Units::EightSleep;
};


class StateManager {
    State state;
    std::shared_ptr<eightsleep::Client> client;

    TaskHandle_t task = nullptr;
    QueueHandle_t queue = nullptr;

public:
    explicit StateManager(const std::shared_ptr<eightsleep::Client>& client);
    ~StateManager();
    void setTargetTemp(int target);
    void setBedState(bool on);
    const State& getState();
    void updateBedState(std::function<void(rd::expected<const State*, std::string>)> cb);

private:
    void enqueue(std::function<void()> fn);
    [[noreturn]] static void taskLoop(void *param);
};

}

#endif //APPS_MAIN_STATE_H
