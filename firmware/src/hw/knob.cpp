#include <Arduino.h>

#include "knob.h"

namespace hw {

void knob::init() {
    pinMode(ROTARY_BUTTON, INPUT_PULLUP);
}

void knob::tick() {
    encoder.tick();

    if (encoder.getPosition() != last_pos) {
        last_pos = encoder.getPosition();
        if (on_rotation) {
            on_rotation();
        }
    }

    PinStatus pin = digitalRead(ROTARY_BUTTON);
    if (pin == PinStatus::HIGH) {
        if (!button_is_down) {
            absolute_time_t now = get_absolute_time();
            if (absolute_time_diff_us(last_button_state_change, now) > 50000) {
                last_button_state_change = now;
                button_is_down = true;
                if (on_button_down) {
                    on_button_down();
                }
            }
        }
    } else if (pin == PinStatus::LOW) {
        if (button_is_down) {
            absolute_time_t now = get_absolute_time();
            if (absolute_time_diff_us(last_button_state_change, now) > 50000) {
                last_button_state_change = now;
                button_is_down = false;
                if (on_button_up) {
                    on_button_up();
                }
                if (on_button_pressed) {
                    on_button_pressed();
                }
            }
        }
    }
}

int knob::direction() {
    return static_cast<int>(encoder.getDirection());
}

int knob::position() {
    return static_cast<int>(encoder.getPosition());
}

}