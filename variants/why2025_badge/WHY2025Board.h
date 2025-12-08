#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>

class WHY2025Board : public ESP32Board {
public:
  void begin() {
    ESP32Board::begin();

#ifdef WHY2025_ENABLE_KEYBOARD_BL
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
#endif

#ifdef WHY2025_ENABLE_DISPLAY_BL
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
#endif
  }

  const char* getManufacturerName() const override {
    return "WHY2025 Badge";
  }
};


