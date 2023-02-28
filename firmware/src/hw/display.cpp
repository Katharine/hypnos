#include <Arduino.h>

#include <Adafruit_GFX.h>

#include "display.h"

namespace hw {

void display::init() {
    SPI.setRX(16);
    SPI.setTX(19);
    SPI.setSCK(18);
    SPI.setCS(17);
    SPI.begin();
    oled.begin(20000000); // 20 MHz.
    clear();
}

void display::clear() {
    oled.fillScreen(0);
    oled.setCursor(0, 0);
    oled.display();
}

void display::print(const std::string& message) {
    oled.print(message.c_str());
    oled.display();
}

}