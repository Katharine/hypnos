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
#include <esp_sntp.h>
#include <statics.h>

namespace apps::main {

namespace {

struct Message {
    std::function<void()> fn;
};

}

// argh. we need a global reference for the SNTP callback.
// we should build a real callback systema round this to avoid it, but for now...
// hack hack hack
static StateManager *theStateManager = nullptr;

StateManager::StateManager() {
    queue = xQueueCreate(10, sizeof(Message *));
    xTaskCreate(&StateManager::taskLoop, "8SlpTask", 8192, this, 5, &task);

    // Claim the time update callback, so we can fix our alarm timer.
    sntp_set_time_sync_notification_cb([](struct timeval *tv) {
        ESP_LOGI(TAG, "time sync! %d", sntp_get_sync_status());
        ESP_LOGI(TAG, "Current time: %ld", static_cast<uint32_t>(time(nullptr)));
        if (theStateManager) {
            theStateManager->handleTimeChange();
        }
    });
    theStateManager = this;

    // Set up a reauth timer
    esp_timer_create_args_t args = {
        .callback = &StateManager::reauthTimerHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "reauth",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &reauthTimerHandle));
    const uint64_t reauth_period = 10 * 3600 * 1000000LL; // 10 hours in microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(reauthTimerHandle, reauth_period));

    // Set up a bed status update timer.
    args = {
        .callback = &StateManager::bedStatusTimerHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "update",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &stateUpdateTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(stateUpdateTimerHandle, 60 * 1000000)); // 1 minute in microseconds

    // Alarm schedule update timer
    args = {
        .callback = &StateManager::alarmScheduleUpdateHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "alarmSchedule",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &regularAlarmUpdateTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(regularAlarmUpdateTimerHandle, 10 * 60 * 1000000)); // 10 minutes in microseconds.

    // Next alarm timer (so we can scream "ALARM" at the right time)
    args = {
        .callback = &StateManager::alarmExpectedHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "alarmNext",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &expectedAlarmTimerHandle));
    // We will start this timer later.

    args = {
        .callback = &StateManager::alarmOngoingPollHandler,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "alarmPoll",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &stillAlarmingPollHandle));

    // Bed state push debounce timer
    args = {
        .callback = &StateManager::handleUpdatePendingTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "updateDebounce",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &updatePendingHandle));
    // We don't start this one here - we'll start it elsewhere.
}

StateManager::~StateManager() {
    theStateManager = nullptr;
    esp_timer_stop(reauthTimerHandle);
    esp_timer_stop(stateUpdateTimerHandle);
    esp_timer_stop(updatePendingHandle);
    esp_timer_stop(regularAlarmUpdateTimerHandle);
    esp_timer_stop(expectedAlarmTimerHandle);
    esp_timer_delete(reauthTimerHandle);
    esp_timer_delete(stateUpdateTimerHandle);
    esp_timer_delete(updatePendingHandle);
    esp_timer_delete(regularAlarmUpdateTimerHandle);
    esp_timer_delete(expectedAlarmTimerHandle);
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
        statics::client->getBedStatus([=, this](rd::expected<eightsleep::Bed, std::string> result) {
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
    ESP_ERROR_CHECK(esp_timer_start_once(updatePendingHandle, 5 * 1000000)); // Five seconds after last change.
}

void StateManager::stopAlarm(std::function<void(rd::expected<bool, std::string>)> cb) {
    enqueue([cb, this]() {
        statics::client->stopAlarms([cb, this](bool result) {
            if (!result) {
                cb(rd::unexpected("stopping alarm failed for some reason"));
            } else {
                state.isAlarming = false;
                cb(true);
            }
        });
    });
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
            statics::client->setTemp(state.localTargetTemp, [this](rd::expected<eightsleep::Bed, std::string> result) {
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
        statics::client->setBedState(state.requestedState, [this, updateTemp](rd::expected<eightsleep::Bed, std::string> result) {
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
        // Don't update the actual temp - we can't get the value from here, so it's garbage.

        serverUpdatePending = false;
        if ((state.requestedState && state.localTargetTemp != state.bedTargetTemp) || state.requestedState != state.bedState) {
            ESP_LOGI(TAG, "States still don't match, requesting another update.");
            setUpdatePendingTimer();
        }

        if (updateCallback) {
            updateCallback(state);
        }
    });
}

void StateManager::reauthTimerHandler(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->enqueue([manager]() {
        statics::client->authenticate(nullptr);
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

void StateManager::alarmScheduleUpdateHandler(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->updateAlarmSchedule(nullptr);
}

void StateManager::alarmExpectedHandler(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->enqueue([manager]() { manager->enterAlarmMode(); });
}

void StateManager::updateAlarmSchedule(const std::function<void(rd::expected<bool, std::string>)>& cb) {
    enqueue([this, cb]() {
        statics::client->getAlarms([this, cb](rd::expected<std::vector<eightsleep::Alarm>, std::string> result) {
            if (!result) {
                ESP_LOGE(TAG, "Failed to update alarms: %s", result.error().c_str());
                if (cb) {
                    cb(rd::unexpected(result.error()));
                }
                return;
            }
            time_t next = INT_LEAST64_MAX;
            time_t now = time(nullptr);
            for (const auto& alarm : *result) {
                time_t scheduled = parseTimeString(alarm.nextTime);
                if (scheduled < next && scheduled > now) {
                    next = scheduled;
                }
            }
            ESP_LOGI(TAG, "Next alarm is scheduled for %d - in %d seconds", (int)next, (int)(next - now));
            if (state.nextAlarm != next) {
                state.nextAlarm = next;
                esp_timer_stop(expectedAlarmTimerHandle);
                ESP_ERROR_CHECK(esp_timer_start_once(expectedAlarmTimerHandle, (next - now) * 1000000));
            }
            if (cb) {
                cb(true);
            }
        });
    });
}

time_t StateManager::parseTimeString(std::string str) {
    struct tm tm = {};
    strptime(str.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
    time_t t = mktime(&tm);
    ESP_LOGI(TAG, "Parsing %s as %d", str.c_str(), (int)t);
    return t;
}

void StateManager::enterAlarmMode() {
    state.isAlarming = true;
    esp_timer_stop(stillAlarmingPollHandle);
    ESP_ERROR_CHECK(esp_timer_start_once(stillAlarmingPollHandle, 5 * 1000000));
    if (updateCallback) {
        updateCallback(state);
    }
}

void StateManager::alarmOngoingPollHandler(void *ctx) {
    auto *manager = static_cast<StateManager*>(ctx);
    manager->enqueue([manager]() { manager->pollActiveAlarm(); });
}

void StateManager::pollActiveAlarm() {
    statics::client->hasActiveAlarm([this](rd::expected<bool, std::string> result) {
        // The expected result is a bool so we have to be explicit.
        if (!result.has_value()) {
            ESP_LOGE(TAG, "Looking up alarm status failed. Generously assuming it is gone.");
        }
        if (result && result.value()) {
            // Schedule another check.
            esp_timer_stop(expectedAlarmTimerHandle); // just to be sure
            ESP_ERROR_CHECK(esp_timer_start_once(expectedAlarmTimerHandle, 5 * 1000000));
            return;
        }
        state.isAlarming = false;
        // TODO: should we move back to our thread for this?
        if (updateCallback) {
            updateCallback(state);
        }
        // Enqueue an alarm schedule check to update the next alarm time.
        updateAlarmSchedule(nullptr);
    });
}

void StateManager::handleTimeChange() {
    esp_timer_stop(expectedAlarmTimerHandle);
    if (state.nextAlarm) {
        time_t timerTime = (state.nextAlarm - time(nullptr));
        ESP_LOGI(TAG, "Updated alarm timer due to time update: %d seconds remain.", (int)timerTime);
        ESP_ERROR_CHECK(esp_timer_start_once(expectedAlarmTimerHandle, timerTime * 1000000));
    }
}

}