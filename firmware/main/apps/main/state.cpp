//
// Created by Katharine Berry on 3/5/23.
//

#include "state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <utility>
#include <esp_log.h>

namespace apps::main {

namespace {

struct Message {
    std::function<void()> fn;
};

}

StateManager::StateManager(const std::shared_ptr<eightsleep::Client>& client) {
    this->client = client;
    queue = xQueueCreate(10, sizeof(Message *));
    xTaskCreate(&StateManager::taskLoop, "8SlpTask", 8192, this, 5, &task);
    // TODO: We need to regularly reauth
}

StateManager::~StateManager() {
    vTaskDelete(task);
    vQueueDelete(queue);
}

[[noreturn]] void StateManager::taskLoop(void *param) {
    auto *state = static_cast<StateManager *>(param);
    while (true) {
        Message* message = nullptr;
        xQueueReceive(state->queue, &message, portMAX_DELAY);
        ESP_LOGI("StateManager", "Received function from queue (%p), executing...", message);
        if (message->fn) {
            message->fn();
        }
        delete message;
    }
}

void StateManager::enqueue(std::function<void()> fn) {
    auto *m = new Message{
        .fn = std::move(fn),
    };
    ESP_LOGI("StateManager", "Sending function to queue (%p)...", m);
    xQueueSendToBack(queue, &m, portMAX_DELAY);
}

void StateManager::updateBedState(std::function<void(rd::expected<const State*, std::string>)> cb) {
    enqueue([=, this]() {
        client->getBedStatus([=, this](rd::expected<eightsleep::Bed, std::string> result) {
            if(!result) {
                cb(rd::unexpected(result.error()));
                return;
            }
            state.bedActualTemp = result->currentTemp;
            state.bedTargetTemp = result->targetTemp;
            state.bedState = result->active;
            state.valid = true;
            cb(&state);
        });
    });
}

const State &StateManager::getState() {
    return state;
}

}