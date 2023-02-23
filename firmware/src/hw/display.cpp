#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "display.h"

namespace hw {

void display::init() {
    Wire.setSDA(0);
    Wire.setSCL(1);
    lcd.init();
    lcd.backlight();
}

void display::clear() {
    lcd.clear();
}

void display::print(const std::string& message) {
    lcd.print(message.c_str());
}

}