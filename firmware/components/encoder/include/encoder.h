#ifndef encoder_include_encoder_h
#define encoder_include_encoder_h

#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <lvgl.h>

namespace encoder {

class Encoder {
    const gpio_num_t a;
    const gpio_num_t b;
    const gpio_num_t s;

    pcnt_unit_handle_t pcnt;
    pcnt_channel_handle_t chan_a;
    pcnt_channel_handle_t chan_b;


public:
    Encoder(int a, int b, int button);
    Encoder(Encoder &&other) noexcept;
    int position();
    bool is_button_pressed();
};

}

#endif
