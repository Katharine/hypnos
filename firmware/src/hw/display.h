#ifndef hw_display_h
#define hw_display_h

#include <string>

#include <Arduino.h>

#include <Adafruit_GFX_Buffer.h>
#include <Adafruit_SSD1351.h>

namespace hw {

class display {
private:
    Adafruit_GFX_Buffer<Adafruit_SSD1351> oled = Adafruit_GFX_Buffer(128, 128, Adafruit_SSD1351(128, 128, &SPI, 17, 21, 20));

public:
    void init();
    void clear();
    void print(const std::string& message);
};

}

#endif