#include <Arduino.h>

#include "ble.h"
#include "config.h"
#include "config_storage.h"
#include "control.h"
#include "fsm.h"
#include "sensors.h"

namespace {

FSM::Machine systemFsm;
Sensors::Readings latestReadings;

unsigned long lastSensorSampleMs = 0;
unsigned long lastBleNotifyMs = 0;

void applyTurnOnRequest() {
  Serial.println("[MAIN] Solicitud de encendido");
  latestReadings = Sensors::readAll();
  systemFsm.update(latestReadings, true);

  if (!systemFsm.isProtectionActive()) {
    Control::setRelay(true);
  } else {
    Serial.println("[MAIN] Encendido bloqueado por PROTECCION");
    Control::setRelay(false);
  }
}

void applyTurnOffRequest() {
  Serial.println("[MAIN] Solicitud de apagado");
  Control::setRelay(false);
}

void handleManualAction() {
  const Control::ManualAction action =
      Control::consumeManualAction(systemFsm.isProtectionActive());

  switch (action) {
    case Control::ManualAction::TurnOn:
      applyTurnOnRequest();
      break;
    case Control::ManualAction::TurnOff:
      applyTurnOffRequest();
      break;
    case Control::ManualAction::None:
    default:
      break;
  }
}

void handleBleCommand() {
  const BLEComms::Command command = BLEComms::consumeCommand();

  switch (command) {
    case BLEComms::Command::TurnOn:
      applyTurnOnRequest();
      break;
    case BLEComms::Command::TurnOff:
      applyTurnOffRequest();
      break;
    case BLEComms::Command::None:
    default:
      break;
  }
}

void sampleSensorsAndUpdateFsm() {
  const unsigned long now = millis();
  if (now - lastSensorSampleMs < Config::SENSOR_SAMPLE_INTERVAL_MS) {
    return;
  }

  lastSensorSampleMs = now;
  latestReadings = Sensors::readAll();
  systemFsm.update(latestReadings, false);

  if (systemFsm.shouldForceRelayOff()) {
    Control::setRelay(false);
  }

  // Actualizar indicadores LED
  const bool eStopPressed = digitalRead(Config::PIN_STOP_BUTTON_NC) == HIGH;
  Control::updateLedIndicators(static_cast<uint8_t>(systemFsm.getState()),
                                Control::estadoSistema, eStopPressed);
}

void notifyBle() {
  const unsigned long now = millis();
  if (now - lastBleNotifyMs < Config::BLE_NOTIFY_INTERVAL_MS) {
    return;
  }

  lastBleNotifyMs = now;

  BLEComms::Telemetry telemetry;
  telemetry.estadoSistema = Control::estadoSistema;
  telemetry.rpm = latestReadings.rpm;
  telemetry.temperatura = latestReadings.temperatureC;
  telemetry.corriente = latestReadings.currentA;
  telemetry.timeOn = now;
  telemetry.alertas = systemFsm.getAlerts();

  BLEComms::sendTelemetry(telemetry);
}

}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUDRATE);

  ConfigStorage::begin();
  const auto config = ConfigStorage::loadConfig();

  Sensors::begin();
  Control::begin();
  systemFsm.begin();
  BLEComms::begin();

  Serial.print("[MAIN] Umbrales activos -> I_nominal=");
  Serial.print(config.I_nominal, 2);
  Serial.print(" I_umbral1=");
  Serial.print(config.I_umbral1, 2);
  Serial.print(" I_umbral2=");
  Serial.print(config.I_umbral2, 2);
  Serial.print(" T_max=");
  Serial.println(config.T_max, 2);

  latestReadings = Sensors::readAll();
  systemFsm.update(latestReadings, false);

  if (systemFsm.shouldForceRelayOff()) {
    Control::setRelay(false);
  }
}

void loop() {
  handleManualAction();
  handleBleCommand();
  sampleSensorsAndUpdateFsm();
  notifyBle();

  delay(10);
}
