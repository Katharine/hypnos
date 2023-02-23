#ifndef hw_display_h
#define hw_display_h

#include <string>

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

namespace hw {

class display {
private:
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x3F, 20, 4);

public:
    void init();
    void clear();
    void print(const std::string& message);
};

}

#endif