#pragma once

#include <Arduino.h>

namespace ConfigStorage {

struct ControlConfig {
  float I_nominal = 0.90f;
  float I_umbral1 = 1.30f;
  float I_umbral1_des = 1.06f;
  float I_umbral2 = 1.50f;
  float I_umbral2_arranque = 3.00f;
  float T_umbral1 = 95.0f;
  float T_umbral1_des = 85.0f;
  float T_max = 105.0f;
};

void begin();
ControlConfig loadConfig();
const ControlConfig& currentConfig();
void saveConfig(const ControlConfig& config);
void resetToDefaults();

}  // namespace ConfigStorage
