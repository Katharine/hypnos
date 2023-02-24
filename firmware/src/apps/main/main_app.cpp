#include <any>
#include <functional>

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiNTP.h>

#include <coreback/coreback.h>
#include <config.h>

#include "main_app.h"

namespace apps {
namespace main {

const pin_size_t TONE_PIN = 6;

void main_app::init() {
    // TODO: get these dynamically.
    client.set_login(HARDCODED_EIGHTSLEEP_USERNAME, HARDCODED_EIGHTSLEEP_PASSWORD);

    knob.on_button_pressed = std::bind(&main_app::knob_pressed, this);
    knob.on_rotation = std::bind(&main_app::knob_rotated, this);

    // Everything wifi must happen on the background core.
    // This call does not (usually) block.
    coreback::run_elsewhere([&](){
        WiFi.begin(HARDCODED_SSID, HARDCODED_WIFI_PASSWORD);

        NTP.begin("time.apple.com");
        NTP.waitSet();

        return client.authenticate();
    }, [&](std::any result) {
        bool authed = std::any_cast<bool>(result);
        if (!authed) {
            // TODO: something?
            display.clear();
            display.print("Authentication failed!");
            Serial.println("auth failed.");
            return;
        }
        display.clear();
        display.print("Authentication successful");
        Serial.println("auth succeeded.");
        ready = true;

        // Re-authenticate every ten hours.
        // TODO: this should be based on the token validity time.
        // Also it should probably live in eightsleep (but then that needs a scheduler reference...)
        scheduler.schedule_repeating_task(36'000'000, [&]() {
            Serial.println("Reauthenticating...");
            coreback::run_elsewhere([&]() {
                client.authenticate();
            });
        });
    });

}

void main_app::tick() {
    // We actually have nothing to do here.
}

void main_app::knob_pressed() {
    if (toning) {
        noTone(TONE_PIN);
    } else {
        tone(TONE_PIN, (knob.position() * 100) + 1000);
    }
    toning = !toning;
    display.clear();
    display.print("Button pressed!");
}

void main_app::knob_rotated() {
    display.clear();
    display.print("pos: " + std::to_string(knob.position()) + ", dir: " + std::to_string(knob.direction()));
    if (toning) {
        tone(TONE_PIN, (knob.position() * 100) + 1000);
    }
}

}
}
