#pragma once

#include <Arduino.h>

namespace BLEComms {

enum class Command : uint8_t {
  None = 0,
  TurnOn,
  TurnOff
};

struct Telemetry {
  bool estadoSistema = false;
  uint32_t rpm = 0;
  float temperatura = 0.0f;
  float corriente = 0.0f;
  uint32_t timeOn = 0;
  uint8_t alertas = 0;
};

void begin();
bool isConnected();
Command consumeCommand();
void sendTelemetry(const Telemetry& telemetry);

}  // namespace BLEComms
