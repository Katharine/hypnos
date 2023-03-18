//
// Created by Katharine Berry on 3/3/23.
//

#include "app.h"
#include "http_cxx.hpp"

#include <lvgl_port.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <eightsleep.h>
#include <statics.h>

using namespace std::placeholders;

namespace apps::config {

void App::present() {
    statics::wifi->setAPConnectCallback([this](bool x) { wifiConnectCallback(x); });
    statics::wifi->startAP();

    presentWiFiUI();

    EventBits_t bits = xEventGroupWaitBits(eventGroup, 1, pdTRUE, pdTRUE, portMAX_DELAY);
    if (!(bits & 1)) {
        ESP_LOGE("App", "Got here, which should be impossible.");
    }
    // Wait a couple of seconds, so we don't tear everything down before the client gets the success message.
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    // Delete the server.
    server.reset();
    statics::wifi->stop();
    statics::wifi->setAPConnectCallback(nullptr);
}

App::App() {
    server = std::make_unique<config_server::Server>();
    eventGroup = xEventGroupCreate();
    server->setCompletionCallback([this](bool success) { completionCallback(success); });
}

App::~App() {
    vEventGroupDelete(eventGroup);
}

void App::presentWiFiUI() {
    auto lock = lvgl_port::Lock();
    wifi_screen = lv_obj_create(nullptr);
    lv_obj_set_size(wifi_screen, lvgl_port::LVGLPort::DISPLAY_WIDTH, lvgl_port::LVGLPort::DISPLAY_HEIGHT);

    icon_label = lv_label_create(wifi_screen);
    lv_label_set_text(icon_label, LV_SYMBOL_WIFI);
    lv_obj_set_align(icon_label, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(icon_label, 0, 20);

    top_label = lv_label_create(wifi_screen);
    lv_label_set_text(top_label, "Join Wi-Fi");
    lv_obj_center(top_label);
    lv_obj_set_pos(top_label, 0, -40);

    static lv_style_t network_name_style;
    lv_style_init(&network_name_style);
    lv_style_set_text_font(&network_name_style, &lv_font_montserrat_32);
    wifi_label = lv_label_create(wifi_screen);
    lv_label_set_text(wifi_label, statics::wifi->getAPSSID().c_str());
    lv_obj_add_style(wifi_label, &network_name_style, 0);
    lv_obj_center(wifi_label);

    bottom_label = lv_label_create(wifi_screen);
    lv_label_set_text(bottom_label, "to begin");
    lv_obj_center(bottom_label);
    lv_obj_set_pos(bottom_label, 0, 45);

    lv_scr_load_anim(wifi_screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::wifiConnectCallback(bool connected) {
    if (connected) {
        presentLinkUI();
    } else {
        presentWiFiUI();
    }
}

void App::presentLinkUI() {
    auto lock = lvgl_port::Lock();
    ESP_LOGI("App", "Free memory: %lu bytes; largest free block: %d bytes.", esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    link_screen = lv_obj_create(nullptr);
    lv_obj_set_size(link_screen, lvgl_port::LVGLPort::DISPLAY_WIDTH, lvgl_port::LVGLPort::DISPLAY_HEIGHT);
    lv_obj_t *qr_code = lv_qrcode_create(link_screen, 160, lv_color_black(), lv_color_white());
    std::string url = "http://" + statics::wifi->getAPIP();
    lv_qrcode_update(qr_code, url.c_str(), url.length());
    lv_obj_center(qr_code);
    // TODO: somehow fit the text on, too.

    lv_scr_load_anim(link_screen, LV_SCR_LOAD_ANIM_FADE_IN, 100, 0, true);
}

void App::completionCallback(bool success) {
    xEventGroupSetBits(eventGroup, 1);
}

}