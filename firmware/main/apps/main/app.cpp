#include "app.h"
#include <lvgl.h>

#include <esp_log.h>
#include <wifi_widget.h>
#include <fonts.h>

namespace apps::main {

void App::present() {
    showConnectingScreen();
    wifi->startNormal([this](bool x) { staConnectCallback(x); });
}

App::App(const std::shared_ptr<lvgl_port::LVGLPort> &port, const std::shared_ptr<wifi::WiFi> &wifi,
         const std::shared_ptr<hypnos_config::HypnosConfig> &config,
         const std::shared_ptr<eightsleep::Client> &client) : port(port), wifi(wifi), config(config), client(client),
                                                              stateManager(client) {

}

void App::staConnectCallback(bool connected) {
    if (connected) {
        client->authenticate([this](bool success) {
           if (success) {
               showFetchingStateScreen();
               stateManager.updateBedState([this](rd::expected<const State*, std::string> result) {
                   if (!result) {
                       ESP_LOGE("app", "Couldn't update state: %s", result.error().c_str());
                       return;
                   }
                   showMainScreen();
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

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_t *wifi_icon = hb_wifi_create(screen);
    lv_obj_center(wifi_icon);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::showMainScreen() {
    auto lock = lvgl_port::Lock();

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    port->setActiveGroup(group);

    lv_obj_t *screen = lv_obj_create(nullptr);
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

            // ...and also:
            auto *app = static_cast<App *>(lv_obj_get_user_data(event->target));
            app->stateManager.setBedState(!app->stateManager.getState().requestedState);
            app->updateMainScreen();
        }
    }, LV_EVENT_KEY, settingArc);

    lv_obj_set_user_data(settingArc, this);

    lv_obj_add_event_cb(settingArc, [](lv_event_t *event) {
        auto lock = lvgl_port::Lock();
        int value = lv_arc_get_value(event->target);
        auto *app = static_cast<App *>(lv_obj_get_user_data(event->target));
        if (app->stateManager.getState().requestedState) {
            app->stateManager.setTargetTemp(value * 10);
            app->updateMainScreen();
        }
    }, LV_EVENT_VALUE_CHANGED, settingArc);

    lv_obj_set_style_text_font(tempLabel, &montserrat_56_digits, 0);
    lv_obj_center(tempLabel);

    lv_obj_set_style_pad_all(settingArc, 0, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(settingArc, 0, LV_PART_INDICATOR);

    lv_group_add_obj(group, settingArc);
    lv_group_focus_obj(settingArc);
    lv_group_set_editing(group, true);

    tempDescLabel = lv_label_create(screen);
    lv_obj_set_style_text_font(tempDescLabel, &lv_font_montserrat_20, 0);
    lv_obj_center(tempDescLabel);
    lv_obj_set_pos(tempDescLabel, 0, 40);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
    stateManager.setUpdateCallback([this](const State&) {
       updateMainScreen();
    });
    updateMainScreen();
}

void App::showConnectionErrorScreen() {
    auto lock = lvgl_port::Lock();

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Connection error.");
    lv_obj_center(label);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::showFetchingStateScreen() {
    auto lock = lvgl_port::Lock();

    lv_obj_t *screen = lv_obj_create(nullptr);

    lv_obj_t *spinner = lv_spinner_create(screen, 1000, 40);
    lv_obj_center(spinner);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::updateMainScreen() {
    auto lock = lvgl_port::Lock();
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

    // Description mapping.
    const char* descriptions[21] = {
        "EXTREMELY COLD",
        "EXTREMELY COLD",
        "EXTREMELY COLD",
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
        "EXTREMELY HOT",
        "EXTREMELY HOT",
        "EXTREMELY HOT",
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

}