#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>

class WHY2025Board : public ESP32Board {
public:
  void begin() {
    ESP32Board::begin();

#ifdef WHY2025_ENABLE_KEYBOARD_BL
    pinMode(10, OUTPUT);
    ledcAttach(10, 25000, 8);
    // unknown (untested) what the minimum is here
    ledcWrite(10, 10);
#endif

#ifdef WHY2025_ENABLE_DISPLAY_BL
    pinMode(15, OUTPUT);
    ledcAttach(15, 25000, 8);
    // 6 is off, 7 is minimum
    ledcWrite(15, 10);
#endif
  }

  const char* getManufacturerName() const override {
    return "WHY2025 Badge";
  }
};


