//
// Created by Katharine Berry on 3/5/23.
//

#include "state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <utility>
#include <esp_log.h>
#include <esp_timer.h>

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

    // Set up a reauth timer
    esp_timer_create_args_t args = {
        .callback = &StateManager::reauthTimerHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "reauth",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&args, &reauthTimerHandle);
    const uint64_t reauth_period = 10 * 3600 * 1000000LL; // 10 hours in microseconds
    esp_timer_start_periodic(reauthTimerHandle, reauth_period);

    // Set up a bed status update timer.
    args = {
        .callback = &StateManager::bedStatusTimerHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "update",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&args, &stateUpdateTimerHandle);
    esp_timer_start_periodic(stateUpdateTimerHandle, 60 * 1000000); // 1 minute in microseconds
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
        ESP_LOGI(TAG, "Received function from queue (%p), executing...", message);
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
    ESP_LOGI(TAG, "Sending function to queue (%p)...", m);
    portBASE_TYPE result = xQueueSendToBack(queue, &m, 2000 / portTICK_PERIOD_MS);
    if (result != pdTRUE) {
        ESP_LOGE(TAG, "Sending to queue failed: %d", result);
    }
}

void StateManager::updateBedState(std::function<void(rd::expected<const State*, std::string>)> cb) {
    enqueue([=, this]() {
        client->getBedStatus([=, this](rd::expected<eightsleep::Bed, std::string> result) {
            if(!result) {
                ESP_LOGI(TAG, "Failed to update bed status: %s", result.error().c_str());
                if (cb) {
                    cb(rd::unexpected(result.error()));
                }
                return;
            }
            bool didChange = false;
            if (state.bedActualTemp != result->currentTemp) {
                state.bedActualTemp = result->currentTemp;
                didChange = true;
            }
            if (state.bedTargetTemp != result->targetTemp) {
                state.bedTargetTemp = result->targetTemp;
                // If something changed remotely, discard our local target temp
                state.localTargetTemp = state.bedTargetTemp;
                didChange = true;
            }
            if (state.bedState != result->active) {
                state.bedState = result->active;
                state.requestedState = state.bedState;
                didChange = true;
            }
            ESP_LOGI(TAG, "Updated bed status. Changed: %d", didChange);
            state.valid = true;
            if (cb) {
                cb(&state);
            }
            if (updateCallback) {
                updateCallback(state);
            }
        });
    });
}

const State &StateManager::getState() {
    return state;
}

void StateManager::reauthTimerHandler(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->enqueue([manager]() {
        manager->client->authenticate(nullptr);
    });
}

void StateManager::bedStatusTimerHandler(void *ctx) {
    auto *manager= static_cast<StateManager*>(ctx);
    // This is entirely asynchronous.
    manager->updateBedState(nullptr);
}

void StateManager::setUpdateCallback(std::function<void(const State&)> cb) {
    updateCallback = std::move(cb);
}

void StateManager::setTargetTemp(int target) {
    state.localTargetTemp = target;
    // TODO: tell the bed about this development.
//    if (updateCallback) {
//        enqueue([this]() {
//            if (updateCallback) {
//                updateCallback(state);
//            }
//        });
//    }
}

void StateManager::setBedState(bool on) {
    state.requestedState = on;
    // TODO: tell the bed about this development.
//    if (updateCallback) {
//        enqueue([this]() {
//            if (updateCallback) {
//                updateCallback(state);
//            }
//        });
//    }
}

}