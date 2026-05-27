#pragma once

#include <Arduino.h>

namespace Control {

enum class ManualAction : uint8_t {
  None = 0,
  TurnOn,
  TurnOff,
  ClearProtection
};

extern bool estadoSistema;

void begin();
void setRelay(bool estado);
bool getRelayState();
int getRelayPinLevel();
bool isEmergencyStopLatched();
ManualAction consumeManualAction(bool protectionActive);
void updateLedIndicators(uint8_t fsmState, bool motorActive, bool emergencyStopLatched);

}  // namespace Control
