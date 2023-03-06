#include "app.h"
#include <lvgl.h>

#include <esp_log.h>
#include <wifi_widget.h>

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
    lv_obj_t *label = lv_label_create(screen);

    lv_obj_add_event_cb(screen, [](lv_event_t* e) {
        ESP_LOGI("app", "Event! %d", e->code);
    }, LV_EVENT_KEY, nullptr);

    lv_obj_t *arc = lv_arc_create(screen);
    lv_obj_set_size(arc, 238, 238);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, -10, 10);
    lv_arc_set_value(arc, stateManager.getState().bedTargetTemp);
    lv_obj_center(arc);

    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(arc, [](lv_event_t *event){
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
    }, LV_EVENT_KEY, arc);

    lv_label_set_text(label, "whee!");
    lv_obj_center(label);

    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(arc, 0, LV_PART_INDICATOR);

    lv_group_add_obj(group, arc);
    lv_group_focus_obj(arc);
    lv_group_set_editing(group, true);

    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
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

}