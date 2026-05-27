#pragma once

#include <Arduino.h>

#include "sensors.h"

namespace FSM {

enum class State : uint8_t {
  NORMAL = 0,
  SOBRECARGA,
  PROTECCION
};

constexpr uint8_t ALERTA_TEMPERATURA = 0b00000001;
constexpr uint8_t ALERTA_RPM = 0b00000010;
constexpr uint8_t ALERTA_CORRIENTE = 0b00000100;

class Machine {
 public:
  void begin();
  void update(const Sensors::Readings& readings, bool manualResetRequested);
  void clearProtection();
  void setMotorActive(bool motorActive);

  State getState() const;
  bool isProtectionActive() const;
  bool shouldForceRelayOff() const;
  uint8_t getAlerts() const;
  const char* getStateName() const;

 private:
  State evaluateOperationalState(const Sensors::Readings& readings);
  bool isProtectionCondition(const Sensors::Readings& readings) const;
  uint8_t buildAlerts(const Sensors::Readings& readings) const;
  bool isCurrentOverloadActive(const Sensors::Readings& readings) const;
  bool isTemperatureOverloadActive(const Sensors::Readings& readings) const;
  bool isRpmAlertActive(const Sensors::Readings& readings) const;
  bool isStartupInhibitWindowActive() const;
  float getCurrentProtectionThreshold() const;

  State state_ = State::NORMAL;
  uint8_t alerts_ = 0;
  bool motorActive_ = false;
  mutable bool currentOverloadLatched_ = false;
  mutable bool temperatureOverloadLatched_ = false;
  unsigned long motorStartMs_ = 0;
};

const char* toString(State state);
String alertsToString(uint8_t alerts);

}  // namespace FSM
