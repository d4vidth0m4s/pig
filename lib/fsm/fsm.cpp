#include "fsm.h"

#include "../../include/config.h"
#include "../config/config_storage.h"

namespace {
}  // namespace

namespace FSM {

void Machine::begin() {
  state_ = State::NORMAL;
  alerts_ = 0;
  motorActive_ = false;
  currentOverloadLatched_ = false;
  temperatureOverloadLatched_ = false;
  motorStartMs_ = 0;
}

void Machine::update(const Sensors::Readings& readings, const bool manualResetRequested) {
  const State previousState = state_;
  const bool protectionCondition = isProtectionCondition(readings);

  if (state_ == State::PROTECCION) {
    if (manualResetRequested && !protectionCondition) {
      state_ = evaluateOperationalState(readings);
    } else {
      state_ = State::PROTECCION;
    }
  } else if (protectionCondition) {
    state_ = State::PROTECCION;
  } else {
    state_ = evaluateOperationalState(readings);
  }

  alerts_ = buildAlerts(readings);

  if (previousState != state_) {
    Serial.print("[FSM] Estado: ");
    Serial.print(toString(previousState));
    Serial.print(" -> ");
    Serial.print(toString(state_));
    Serial.print(" | I=");
    Serial.print(readings.currentA, 2);
    Serial.print("A | T=");
    Serial.print(readings.temperatureC, 2);
    Serial.print("C | alertas=");
    Serial.print(static_cast<uint32_t>(alerts_));
    Serial.print(" (");
    Serial.print(alertsToString(alerts_));
    Serial.println(")");
  }
}

void Machine::clearProtection() {
  if (state_ != State::PROTECCION) {
    return;
  }

  Serial.println("[FSM] PROTECCION liberada manualmente por boton NC");
  state_ = State::NORMAL;
  alerts_ = 0;
}

void Machine::setMotorActive(const bool motorActive) {
  if (motorActive == motorActive_) {
    return;
  }

  motorActive_ = motorActive;
  if (motorActive_) {
    motorStartMs_ = millis();
    Serial.print("[FSM] Ventana de arranque habilitada por ");
    Serial.print(Config::STARTUP_INHIBIT_WINDOW_MS);
    Serial.println(" ms");
    return;
  }

  currentOverloadLatched_ = false;
  temperatureOverloadLatched_ = false;
  motorStartMs_ = 0;
}

State Machine::evaluateOperationalState(const Sensors::Readings& readings) {
  if (isCurrentOverloadActive(readings) || isTemperatureOverloadActive(readings) ||
      isRpmAlertActive(readings)) {
    return State::SOBRECARGA;
  }

  return State::NORMAL;
}

bool Machine::isProtectionCondition(const Sensors::Readings& readings) const {
  const auto& config = ConfigStorage::currentConfig();
  const bool currentProtection =
      motorActive_ && readings.currentA > getCurrentProtectionThreshold();
  const bool temperatureProtection = readings.temperatureC > config.T_max;
  const bool rpmProtection =
      motorActive_ && !isStartupInhibitWindowActive() &&
      readings.rpm <= Config::RPM_PROTECTION_THRESHOLD &&
      readings.currentA > Config::RPM_PROTECTION_CURRENT_THRESHOLD;
  return currentProtection || temperatureProtection || rpmProtection;
}

uint8_t Machine::buildAlerts(const Sensors::Readings& readings) const {
  uint8_t result = 0;

  if (isTemperatureOverloadActive(readings) ||
      readings.temperatureC > ConfigStorage::currentConfig().T_max) {
    result |= ALERTA_TEMPERATURA;
  }

  if (isRpmAlertActive(readings) ||
      (motorActive_ && !isStartupInhibitWindowActive() &&
       readings.rpm <= Config::RPM_PROTECTION_THRESHOLD &&
       readings.currentA > Config::RPM_PROTECTION_CURRENT_THRESHOLD)) {
    result |= ALERTA_RPM;
  }

  if (isCurrentOverloadActive(readings) ||
      (motorActive_ && readings.currentA > getCurrentProtectionThreshold())) {
    result |= ALERTA_CORRIENTE;
  }

  return result;
}

bool Machine::isCurrentOverloadActive(const Sensors::Readings& readings) const {
  if (!motorActive_) {
    return false;
  }

  const auto& config = ConfigStorage::currentConfig();
  if (readings.currentA > config.I_umbral1) {
    currentOverloadLatched_ = true;
  } else if (currentOverloadLatched_ && readings.currentA < config.I_umbral1_des) {
    currentOverloadLatched_ = false;
  }

  return currentOverloadLatched_;
}

bool Machine::isTemperatureOverloadActive(const Sensors::Readings& readings) const {
  const auto& config = ConfigStorage::currentConfig();
  if (readings.temperatureC > config.T_umbral1) {
    temperatureOverloadLatched_ = true;
  } else if (temperatureOverloadLatched_ &&
             readings.temperatureC < config.T_umbral1_des) {
    temperatureOverloadLatched_ = false;
  }

  return temperatureOverloadLatched_;
}

bool Machine::isRpmAlertActive(const Sensors::Readings& readings) const {
  if (!motorActive_ || isStartupInhibitWindowActive()) {
    return false;
  }

  return readings.rpm < Config::RPM_ALERT_THRESHOLD &&
         readings.currentA > Config::RPM_ALERT_CURRENT_THRESHOLD;
}

bool Machine::isStartupInhibitWindowActive() const {
  if (!motorActive_) {
    return false;
  }

  return millis() - motorStartMs_ < Config::STARTUP_INHIBIT_WINDOW_MS;
}

float Machine::getCurrentProtectionThreshold() const {
  const auto& config = ConfigStorage::currentConfig();
  return isStartupInhibitWindowActive() ? config.I_umbral2_arranque : config.I_umbral2;
}

State Machine::getState() const { return state_; }

bool Machine::isProtectionActive() const { return state_ == State::PROTECCION; }

bool Machine::shouldForceRelayOff() const { return isProtectionActive(); }

uint8_t Machine::getAlerts() const { return alerts_; }

const char* Machine::getStateName() const { return toString(state_); }

const char* toString(const State state) {
  switch (state) {
    case State::NORMAL:
      return "NORMAL";
    case State::SOBRECARGA:
      return "SOBRECARGA";
    case State::PROTECCION:
      return "PROTECCION";
    default:
      return "DESCONOCIDO";
  }
}

String alertsToString(const uint8_t alerts) {
  if (alerts == 0) {
    return "sin_alertas";
  }

  String result;

  if ((alerts & ALERTA_TEMPERATURA) != 0) {
    result += "temperatura";
  }

  if ((alerts & ALERTA_RPM) != 0) {
    if (!result.isEmpty()) {
      result += ", ";
    }
    result += "rpm";
  }

  if ((alerts & ALERTA_CORRIENTE) != 0) {
    if (!result.isEmpty()) {
      result += ", ";
    }
    result += "corriente";
  }

  return result;
}

}  // namespace FSM
