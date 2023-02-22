#include <Arduino.h>
#include <WiFi.h>
#include <rotary_encoder.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiNTP.h>
#include <ctime>
#include <locale>
#include <sstream>

#include "eightsleep/eightsleep.h"
#include "eightsleep/alarm.h"

const pin_size_t ROTARY_ENCODER_A = 3;
const pin_size_t ROTARY_ENCODER_B = 2;
const pin_size_t ROTARY_BUTTON = 4;
const pin_size_t TONE_PIN = 6;

RotaryEncoder encoder(ROTARY_ENCODER_A, ROTARY_ENCODER_B, RotaryEncoder::LatchMode::TWO03);
LiquidCrystal_I2C display(0x3F, 20, 4);

void setup() {
  Wire.setSCL(1);
  Wire.setSDA(0);
  Serial.begin();
  display.init();
  display.backlight();

  int status = WL_IDLE_STATUS;

  display.clear();
  display.print("Attempting to connect to WiFi...");
  status = WiFi.begin("ssid", "password");
  display.clear();
  display.print("Connected! ");
  display.print(WiFi.localIP().toString());
  NTP.begin("time.apple.com");
  NTP.waitSet();
  display.clear();
  display.print("Time set! ");
  time_t t = time(nullptr);
  char ts[50];
  strftime(ts, 50, "%c", localtime(&t));
  display.print(ts);

  eightsleep::eightsleep foo("user@example.com", "password123");
  foo.authenticate();
  auto alarms = foo.get_alarms();
  if (!alarms) {
    Serial.print("Fetching alarms failed: ");
    Serial.println(alarms.error().c_str());
  } else {
    for (const auto& a : *alarms) {
      Serial.printf("Alarm %s will next fire on %s.\n", a.id.c_str(), a.next_time.c_str());
    }
  }
  
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ROTARY_BUTTON, INPUT_PULLUP);
}

void loop() {
  static long pos = 0;
  static bool button_pressed = false;
  static bool toning = false;
  static int tone_freq = 1000;
  encoder.tick();
  long newPos = encoder.getPosition();
  if (pos != newPos) {
    display.clear();
    display.print("pos:");
    display.print(newPos);
    display.print(" dir:");
    display.print((int)(encoder.getDirection()));
    pos = newPos;
    tone_freq = newPos * 100 + 1000;
    if (toning) {
      noTone(TONE_PIN);
      tone(TONE_PIN, tone_freq);
    }
  }
  byte button_state = digitalRead(ROTARY_BUTTON);
  if (button_state == 0 && !button_pressed) {
    display.clear();
    Serial.print("Button pressed!");
    display.print("Button pressed!");
    button_pressed = true;
    toning = !toning;
    if (!toning) {
      noTone(TONE_PIN);
    } else {
      tone(TONE_PIN, tone_freq);
    }
    display.print(toning);

  } else if (button_state == 1) {
    if (button_pressed) {
      Serial.print("Button released!");
      delay(20);
    }
    button_pressed = false;
  }
}