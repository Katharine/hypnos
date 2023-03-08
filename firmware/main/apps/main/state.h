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
#include <esp_timer.h>

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
    static constexpr char const * TAG = "StateManager";
    State state;
    std::shared_ptr<eightsleep::Client> client;
    bool serverUpdatePending = false;

    TaskHandle_t task = nullptr;
    QueueHandle_t queue = nullptr;

    esp_timer_handle_t reauthTimerHandle;
    esp_timer_handle_t stateUpdateTimerHandle;
    esp_timer_handle_t updatePendingHandle;

    std::function<void(const State&)> updateCallback = nullptr;

public:
    explicit StateManager(const std::shared_ptr<eightsleep::Client>& client);
    ~StateManager();
    void setTargetTemp(int target);
    void setBedState(bool on);
    const State& getState();
    void updateBedState(std::function<void(rd::expected<const State*, std::string>)> cb);
    void setUpdateCallback(std::function<void(const State&)> cb);
    void recheckState(const eightsleep::Bed& newState);

private:
    void enqueue(std::function<void()> fn);
    void setUpdatePendingTimer();
    void syncStateToServer();

    [[noreturn]] static void taskLoop(void *param);
    static void reauthTimerHandler(void *ctx);
    static void bedStatusTimerHandler(void *ctx);
    static void handleUpdatePendingTimer(void *ctx);
};

}

#endif //APPS_MAIN_STATE_H
