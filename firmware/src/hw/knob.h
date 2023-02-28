#ifndef hw_knob_h
#define hw_knob_h

#include <functional>
#include <rotary_encoder.h>

namespace hw {

class knob {
public:
    typedef std::function<void()> button_callback;
    button_callback on_button_down;
    button_callback on_button_up;
    button_callback on_button_pressed;
    button_callback on_rotation;

private:
    const pin_size_t ROTARY_ENCODER_A = 11;
    const pin_size_t ROTARY_ENCODER_B = 10;
    const pin_size_t ROTARY_BUTTON = 12;

    int last_pos = 0;
    absolute_time_t last_button_state_change;
    

    RotaryEncoder encoder = RotaryEncoder(ROTARY_ENCODER_A, ROTARY_ENCODER_B, RotaryEncoder::LatchMode::TWO03);

    bool button_is_down = false;

public:
    void init();
    void tick();
    int direction();
    int position();

};

}

#endif