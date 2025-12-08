#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>

class WHY2025Board : public ESP32Board {
public:
  void begin() {
    ESP32Board::begin();
  }

  const char* getManufacturerName() const override {
    return "WHY2025 Badge";
  }
};


