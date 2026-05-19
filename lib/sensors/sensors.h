#pragma once

#include <Arduino.h>

namespace Sensors {

struct Readings {
  float temperatureC = 0.0f;
  float currentA = 0.0f;
  uint32_t rpm = 0;
};

void begin();
Readings readAll();
float readTemperatureC();
float readCurrentA();
uint32_t readRPM();

}  // namespace Sensors
