#pragma once

#include <Arduino.h>

namespace ConfigStorage {

struct ControlConfig {
  float I_nominal = 2.0f;
  float I_umbral1 = 4.0f;
  float I_umbral2 = 5.5f;
  float T_max = 60.0f;
};

void begin();
ControlConfig loadConfig();
const ControlConfig& currentConfig();
void saveConfig(const ControlConfig& config);
void resetToDefaults();

}  // namespace ConfigStorage
