#include "encoder.h"

#include <functional>

#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <esp_log.h>

namespace encoder {

Encoder::Encoder(int a, int b, int button) : a(static_cast<gpio_num_t>(a)), b(static_cast<gpio_num_t>(b)), s(static_cast<gpio_num_t>(button)) {
     gpio_config_t gpiconfig{
         .pin_bit_mask = (1ULL << s),
         .mode = GPIO_MODE_INPUT,
         .pull_up_en = GPIO_PULLUP_ENABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_ANYEDGE,
     };
    ESP_ERROR_CHECK(gpio_config(&gpiconfig));
    // gpio_isr_handler_add(this->a, &handle_isr, this);
    // gpio_isr_handler_add(this->b, &handle_isr, this);
    // gpio_set_direction(this->a, GPIO_MODE_INPUT);
    // gpio_set_direction(this->b, GPIO_MODE_INPUT);

    pcnt_unit_config_t config = {
        .low_limit = -32736,
        .high_limit = 32736,
        .flags = {
            .accum_count = true,
        },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&config, &pcnt));
    pcnt_glitch_filter_config_t glitch_config = {
        .max_glitch_ns = 12000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt, &glitch_config));
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = a,
        .level_gpio_num = b,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt, &chan_a_config, &chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = b,
        .level_gpio_num = a,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt, &chan_b_config, &chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt, config.low_limit));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt, config.high_limit));

    pcnt_unit_enable(pcnt);
    pcnt_unit_clear_count(pcnt);
    pcnt_unit_start(pcnt);
}

int Encoder::position() {
    int count;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt, &count));
    return count / 4;
}

bool Encoder::is_button_pressed() {
    int button_state = gpio_get_level(s);
    return !button_state;
}

Encoder::Encoder(Encoder &&other) noexcept : a(other.a), b(other.b), s(other.s) {
    pcnt = other.pcnt;
    chan_a = other.chan_a;
    chan_b = other.chan_b;
    other.pcnt = nullptr;
    other.chan_a = nullptr;
    other.chan_b = nullptr;
}

}