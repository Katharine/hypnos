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

    args = {
        .callback = &StateManager::handleUpdatePendingTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "updateDebounce",
        .skip_unhandled_events = false,
    };
    esp_timer_create(&args, &updatePendingHandle);
    // We don't start this one here - we'll start it elsewhere.
}

StateManager::~StateManager() {
    esp_timer_stop(reauthTimerHandle);
    esp_timer_stop(stateUpdateTimerHandle);
    esp_timer_stop(updatePendingHandle);
    esp_timer_delete(reauthTimerHandle);
    esp_timer_delete(stateUpdateTimerHandle);
    esp_timer_delete(updatePendingHandle);
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

void StateManager::setUpdateCallback(std::function<void(const State&)> cb) {
    updateCallback = std::move(cb);
}

void StateManager::setTargetTemp(int target) {
    state.localTargetTemp = target;
    setUpdatePendingTimer();
}

void StateManager::setBedState(bool on) {
    state.requestedState = on;
    setUpdatePendingTimer();
}

void StateManager::setUpdatePendingTimer() {
    esp_timer_stop(updatePendingHandle);
    esp_timer_start_once(updatePendingHandle, 5 * 1000000); // Five seconds after last change.
}

void StateManager::syncStateToServer() {
    if (serverUpdatePending) {
        ESP_LOGI(TAG, "Skipped sync to server: already working on it");
        return;
    }
    if (state.localTargetTemp == state.bedTargetTemp && state.requestedState == state.bedState) {
        ESP_LOGI(TAG, "Skipped sync to server: we don't think we need to change anything");
        return;
    }
    ESP_LOGI(TAG, "Syncing updates to server...");
    serverUpdatePending = true;
    std::function<void()> updateTemp = nullptr;
    if (state.localTargetTemp != state.bedTargetTemp) {
        updateTemp = [this]() {
            client->setTemp(state.localTargetTemp, [this](rd::expected<eightsleep::Bed, std::string> result) {
                if (!result) {
                    ESP_LOGW(TAG, "Temperature update failed: %s", result.error().c_str());
                    state.localTargetTemp = state.bedTargetTemp;
                    state.requestedState = state.bedState;
                    serverUpdatePending = false;
                    if (updateCallback) {
                        updateCallback(state);
                    }
                    return;
                }
                ESP_LOGI(TAG, "Temperature update complete.");
                recheckState(result.value());
            });
        };
    }
    if (state.bedState != state.requestedState) {
        client->setBedState(state.requestedState, [this, updateTemp](rd::expected<eightsleep::Bed, std::string> result) {
            if (!result) {
                ESP_LOGW(TAG, "State update failed: %s", result.error().c_str());
                state.localTargetTemp = state.bedTargetTemp;
                state.requestedState = state.bedState;
                serverUpdatePending = false;
                if (updateCallback) {
                    updateCallback(state);
                }
                return;
            }
            ESP_LOGI(TAG, "State update complete.");
            if (updateTemp) {
                updateTemp();
            } else {
                recheckState(result.value());
            }
        });
    } else if (updateTemp) {
        updateTemp();
    } else {
        // This should be impossible.
        ESP_LOGE(TAG, "Reached unreachable code for serverUpdatePending.");
        serverUpdatePending = false;
    }
}

void StateManager::recheckState(const eightsleep::Bed &newState) {
    enqueue([=]() {
        state.bedTargetTemp = newState.targetTemp;
        state.bedState = newState.active;
        state.bedActualTemp = newState.currentTemp;

        serverUpdatePending = false;
        if ((state.requestedState && state.localTargetTemp != state.bedTargetTemp) || state.requestedState != state.bedState) {
            ESP_LOGI(TAG, "States still don't match, requesting another update.");
            setUpdatePendingTimer();
        }
    });
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

void StateManager::handleUpdatePendingTimer(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->enqueue([manager]() { manager->syncStateToServer(); });
}

}