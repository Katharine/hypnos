#include "app.h"
#include <lvgl.h>

#include <esp_log.h>
#include <wifi_widget.h>
#include <temp_ticks.h>
#include <fonts.h>
#include <statics.h>

namespace apps::main {

void App::present() {
    showConnectingScreen();
    statics::statics.wifi->startNormal([this](bool x) { staConnectCallback(x); });
}

void App::staConnectCallback(bool connected) {
    if (connected) {
        statics::statics.client->authenticate([this](bool success) {
           if (success) {
               showFetchingStateScreen();
               stateManager.updateBedState([this](rd::expected<const State*, std::string> result) {
                   if (!result) {
                       ESP_LOGE("app", "Couldn't update state: %s", result.error().c_str());
                       showConnectionErrorScreen();
                       return;
                   }
                   stateManager.updateAlarmSchedule([this](rd::expected<bool, std::string> alarmResult) {
                       if (!alarmResult) {
                           ESP_LOGE("app", "Couldn't update alarms: %s", alarmResult.error().c_str());
                           showConnectionErrorScreen();
                           return;
                       }
                       showMainScreen();
                   });
               });
           } else {
               showConnectionErrorScreen();
           }
        });
    } else {
        showConnectionErrorScreen();
    }
}

void App::showConnectingScreen() {
    auto lock = lvgl_port::Lock();
    currentScreen = Screen::Connecting;

    lv_obj_t *screen = statics::statics.screenStack->createScreen();
    lv_obj_t *wifi_icon = hb_wifi_create(screen);
    lv_obj_center(wifi_icon);

    statics::statics.screenStack->replace(screen);
}

void App::showMainScreen() {
    auto lock = lvgl_port::Lock();
    currentScreen = Screen::Main;

    lv_obj_t *screen = statics::statics.screenStack->createScreen([this]() {
        currentScreen = Screen::Main;
    });

    tempLabel = lv_label_create(screen);

    lv_obj_add_event_cb(screen, [](lv_event_t* e) {
        ESP_LOGI("app", "Event! %d", e->code);
    }, LV_EVENT_KEY, nullptr);

    settingArc = lv_arc_create(screen);
    lv_obj_set_size(settingArc, 238, 238);
    lv_arc_set_rotation(settingArc, 135);
    lv_arc_set_bg_angles(settingArc, 0, 270);
    lv_arc_set_range(settingArc, -10, 10);
    lv_obj_center(settingArc);

    lv_obj_clear_flag(settingArc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(settingArc, [](lv_event_t *event){
        auto *app = static_cast<App *>(lv_obj_get_user_data(event->target));
        if (app->currentScreen != Screen::Main) {
            return;
        }
        auto lock = lvgl_port::Lock();
        uint32_t key = lv_event_get_key(event);
        if (key == LV_KEY_ENTER) {
            // When the user presses the button ("enter"), we get taken out of 'edit mode'
            // and so can no longer react to anything.
            auto *arc = static_cast<lv_obj_t *>(event->user_data);
            ESP_LOGI("app", "defocused, refocusing. current focus: %p",
                     lv_group_get_focused((lv_group_t *) lv_obj_get_group(arc)));
            lv_group_focus_obj(arc);
            lv_group_set_editing((lv_group_t *) lv_obj_get_group(arc), true);
        }
    }, LV_EVENT_KEY, settingArc);

    lv_obj_add_event_cb(settingArc, [](lv_event_t *event) {
        auto *app = static_cast<App *>(lv_event_get_user_data(event));
        // We apparently don't have a working LV_EVENT_LONG_PRESSED (why?), so instead we start a timer here.
        if (app->currentScreen != Screen::Main) {
            return;
        }
        esp_timer_stop(app->longPressTimer);
        esp_timer_start_once(app->longPressTimer, 400 * 1000);
    }, LV_EVENT_PRESSED, this);

    lv_obj_add_event_cb(settingArc, [](lv_event_t *event) {
        auto *app = static_cast<App *>(lv_event_get_user_data(event));
        if (app->currentScreen != Screen::Main) {
            return;
        }
        esp_timer_stop(app->longPressTimer);
        app->stateManager.setBedState(!app->stateManager.getState().requestedState);
        app->updateMainScreen();
    }, LV_EVENT_RELEASED, this);

    esp_timer_create_args_t args = {
        .callback = [](void* arg) {
            // Long press callback;
            auto *app = static_cast<App *>(arg);
            if (app->currentScreen != Screen::Main) {
                return;
            }
            app->currentScreen = Screen::Settings;
            app->menu.display();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "LongPressTimer",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &longPressTimer));

    lv_obj_set_user_data(settingArc, this);

    lv_obj_add_event_cb(settingArc, [](lv_event_t *event) {
        auto *app = static_cast<App *>(lv_obj_get_user_data(event->target));
        if (app->currentScreen != Screen::Main) {
            return;
        }
        auto lock = lvgl_port::Lock();
        int value = lv_arc_get_value(event->target);
        if (app->stateManager.getState().requestedState) {
            app->stateManager.setTargetTemp(value * 10);
            app->updateMainScreen();
        }
    }, LV_EVENT_VALUE_CHANGED, settingArc);

    lv_obj_set_style_text_font(tempLabel, &montserrat_56_digits, 0);
    lv_obj_center(tempLabel);

    lv_obj_set_style_pad_all(settingArc, 0, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(settingArc, 0, LV_PART_INDICATOR);

    lv_group_t *group = statics::statics.screenStack->groupForScreen(screen);
    lv_group_add_obj(group, settingArc);
    lv_group_focus_obj(settingArc);
    lv_group_set_editing(group, true);

    tempDescLabel = lv_label_create(screen);
    lv_obj_set_style_text_font(tempDescLabel, &lv_font_montserrat_20, 0);
    lv_obj_center(tempDescLabel);
    lv_obj_set_pos(tempDescLabel, 0, 40);

    tickCircle = hb_ticks_create(screen);
    hb_ticks_set_rotation(tickCircle, 135);
    hb_ticks_set_range(tickCircle, -10, 10);
    hb_ticks_set_angles(tickCircle, 0, 270);
    lv_obj_set_size(tickCircle, 200, 200);
    lv_obj_center(tickCircle);

    statics::statics.screenStack->replace(screen);
    stateManager.setUpdateCallback([this](const State& state) {
        handleStateUpdate(state);
    });
    updateMainScreen();
}

void App::handleStateUpdate(const State& state) {
    // TODO: figure out what the actually acceptable state transitions here are.
    //       Also maybe encode them better.
    if (state.isAlarming) {
        if (currentScreen != Screen::Alarm && currentScreen != Screen::FetchingState) {
            showAlarmScreen();
            return;
        }
    } else {
        if (currentScreen == Screen::Alarm || currentScreen == Screen::FetchingState) {
            showMainScreen();
            return;
        }
        updateMainScreen();
    }
}

void App::showConnectionErrorScreen() {
    auto lock = lvgl_port::Lock();
    currentScreen = Screen::ConnectionError;

    lv_obj_t *screen = statics::statics.screenStack->createScreen();
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Connection error.");
    lv_obj_center(label);

    statics::statics.screenStack->replace(screen);
}

void App::showFetchingStateScreen() {
    auto lock = lvgl_port::Lock();
    currentScreen = Screen::FetchingState;

    lv_obj_t *screen = statics::statics.screenStack->createScreen();

    lv_obj_t *spinner = lv_spinner_create(screen, 1000, 40);
    lv_obj_center(spinner);

    statics::statics.screenStack->replace(screen);
}

void App::updateMainScreen() {
    auto lock = lvgl_port::Lock();
    if (currentScreen != Screen::Main) {
        return;
    }
    constexpr size_t maxLength = 4;
    char text[maxLength] = {};
    int temp = LV_CLAMP(-10, stateManager.getState().localTargetTemp / 10, 10);
    if (!stateManager.getState().requestedState) {
        strlcpy(text, "OFF", maxLength);
    } else if (temp == 0) {
        text[0] = '0';
    } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" // GCC doesn't know this is clamped between -10 and +10.
        snprintf(text, maxLength, "%+d", temp);
#pragma GCC diagnostic pop
    }
    lv_label_set_text(tempLabel, text);
    if (stateManager.getState().requestedState) {
        lv_obj_set_style_opa(settingArc, LV_OPA_100, LV_PART_KNOB);
        lv_arc_set_value(settingArc, temp);
    } else {
        lv_obj_set_style_opa(settingArc, LV_OPA_0, LV_PART_KNOB);
    }

    if (stateManager.getState().requestedState) {
        lv_obj_set_style_opa(tickCircle, LV_OPA_COVER, LV_PART_MAIN);
        hb_ticks_set_requested_target(tickCircle, stateManager.getState().localTargetTemp / 10);
        hb_ticks_set_true_target(tickCircle, stateManager.getState().bedTargetTemp / 10);
        hb_ticks_set_actual(tickCircle, stateManager.getState().bedActualTemp / 10);
    } else {
        lv_obj_set_style_opa(tickCircle, LV_OPA_TRANSP, LV_PART_MAIN);
    }

    // Description mapping.
    const char* descriptions[21] = {
        "SUPER COLD",
        "SUPER COLD",
        "SUPER COLD",
        "VERY COLD",
        "VERY COLD",
        "VERY COLD",
        "COLD",
        "COLD",
        "COOL",
        "COOL",
        "NEUTRAL",
        "WARM",
        "WARM",
        "HOT",
        "HOT",
        "VERY HOT",
        "VERY HOT",
        "VERY HOT",
        "SUPER HOT",
        "SUPER HOT",
        "SUPER HOT",
    };
    const lv_color_t colors[21] = {
        lv_color_hex(0x1862FF),
        lv_color_hex(0x1862FF),
        lv_color_hex(0x1862FF),
        lv_color_hex(0x304BF6),
        lv_color_hex(0x304BF6),
        lv_color_hex(0x304BF6),
        lv_color_hex(0x3744F3),
        lv_color_hex(0x3744F3),
        lv_color_hex(0x4C39F2),
        lv_color_hex(0x4C39F2),
        lv_color_hex(0x832EF5),
        lv_color_hex(0xB91332),
        lv_color_hex(0xB91332),
        lv_color_hex(0xC31435),
        lv_color_hex(0xC31435),
        lv_color_hex(0xD01639),
        lv_color_hex(0xD01639),
        lv_color_hex(0xD01639),
        lv_color_hex(0xE6183F),
        lv_color_hex(0xE6183F),
        lv_color_hex(0xE6183F),
    };
    if (stateManager.getState().requestedState) {
        lv_label_set_text(tempDescLabel, descriptions[temp + 10]);
        lv_obj_set_style_text_color(tempDescLabel, colors[temp + 10], 0);
        lv_obj_set_style_bg_color(settingArc, colors[temp + 10], LV_PART_KNOB);
    } else {
        lv_label_set_text(tempDescLabel, "");
    }
}

void App::showAlarmScreen() {
    auto lock = lvgl_port::Lock();
    currentScreen = Screen::Alarm;

    lv_obj_t *screen = statics::statics.screenStack->createScreen();
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "ALARM");
    lv_obj_set_style_text_font(label, &montserrat_56_digits, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *button = lv_btn_create(screen);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 50);

    lv_obj_t *buttonLabel = lv_label_create(button);
    lv_label_set_text(buttonLabel, "Dismiss");
    lv_obj_center(buttonLabel);

    lv_obj_add_event_cb(button, [](lv_event_t *event) {
        auto lock = lvgl_port::Lock();
        auto app = static_cast<App*>(event->user_data);
        if (app->currentScreen != Screen::Alarm) {
            return;
        }
        app->showFetchingStateScreen();
        app->stateManager.stopAlarm([app](rd::expected<bool, std::string> result) {
            auto lock = lvgl_port::Lock();
            if (!result) {
                app->showAlarmScreen();
            } else {
                app->showMainScreen();
            }
        });

    }, LV_EVENT_RELEASED, this);

    statics::statics.screenStack->replace(screen);
}

}