#include "control.h"

#include <driver/gpio.h>

#include "../../include/config.h"
#include "../fsm/fsm.h"
#include "utils.h"

namespace {

Utils::Debouncer startButtonDebouncer(Config::BUTTON_DEBOUNCE_MS);
Utils::Debouncer stopButtonDebouncer(Config::BUTTON_DEBOUNCE_MS);

gpio_num_t relayGpio = static_cast<gpio_num_t>(Config::PIN_RELAY);

bool isStartButtonPressed() {
  return digitalRead(Config::PIN_START_BUTTON_NO) == LOW;
}

bool isStopButtonOpen() {
  return digitalRead(Config::PIN_STOP_BUTTON_NC) == HIGH;
}

void writeRelayPin(const bool estado) {
  const uint8_t level = estado ? Config::RELAY_ACTIVE_LEVEL : Config::RELAY_INACTIVE_LEVEL;
  digitalWrite(Config::PIN_RELAY, level);
  gpio_set_level(relayGpio, level == HIGH ? 1 : 0);
}

int readRelayPinLevel() {
  return gpio_get_level(relayGpio);
}

int expectedRelayPinLevel(const bool estado) {
  const uint8_t level = estado ? Config::RELAY_ACTIVE_LEVEL : Config::RELAY_INACTIVE_LEVEL;
  return level == HIGH ? 1 : 0;
}

unsigned long lastLedUpdateMs = 0;
bool ledBlinkState = false;
bool emergencyStopLatched = false;

}  // namespace

namespace Control {

bool estadoSistema = false;

void begin() {
  gpio_reset_pin(relayGpio);
  gpio_set_direction(relayGpio, GPIO_MODE_OUTPUT);
  pinMode(Config::PIN_RELAY, OUTPUT);
  pinMode(Config::PIN_START_BUTTON_NO, INPUT_PULLUP);
  pinMode(Config::PIN_STOP_BUTTON_NC, INPUT_PULLUP);
  pinMode(Config::PIN_LED_GREEN, OUTPUT);
  pinMode(Config::PIN_LED_RED, OUTPUT);
  writeRelayPin(false);

  Serial.print("[CONTROL] Relay pin inicializado GPIO");
  Serial.print(Config::PIN_RELAY);
  Serial.print(" nivel=");
  Serial.println(readRelayPinLevel());

  Serial.print("[CONTROL] Boton START (NO) en GPIO");
  Serial.println(Config::PIN_START_BUTTON_NO);
  Serial.print("[CONTROL] Boton STOP (NC) en GPIO");
  Serial.println(Config::PIN_STOP_BUTTON_NC);

  digitalWrite(Config::PIN_LED_GREEN, LOW);
  digitalWrite(Config::PIN_LED_RED, LOW);

  setRelay(false);
}

void setRelay(const bool estado) {
  estadoSistema = estado;
  writeRelayPin(estado);

  const int pinLevel = readRelayPinLevel();

  Serial.print("[CONTROL] Relay ");
  Serial.print(estadoSistema ? "ON" : "OFF");
  Serial.print(" | GPIO");
  Serial.print(Config::PIN_RELAY);
  Serial.print("=");
  Serial.println(pinLevel == 1 ? "HIGH" : "LOW");

  if (pinLevel != expectedRelayPinLevel(estadoSistema)) {
    Serial.println("[CONTROL] Advertencia: el nivel leido del pin no coincide con el estado solicitado");
  }
}

bool getRelayState() { return estadoSistema; }

int getRelayPinLevel() { return readRelayPinLevel(); }

bool isEmergencyStopLatched() { return emergencyStopLatched; }

ManualAction consumeManualAction(const bool protectionActive) {
  const uint32_t now = millis();

  stopButtonDebouncer.update(isStopButtonOpen(), now);
  if (stopButtonDebouncer.rose()) {
    if (emergencyStopLatched) {
      emergencyStopLatched = false;
      Serial.println("[CONTROL] STOP NC: salida de parada de emergencia -> modo reposo");
      return ManualAction::TurnOff;
    }

    if (protectionActive) {
      Serial.println("[CONTROL] STOP NC: liberando estado PROTECCION -> modo reposo");
      return ManualAction::ClearProtection;
    }

    if (estadoSistema) {
      emergencyStopLatched = true;
      Serial.println("[CONTROL] STOP NC: parada de emergencia activada");
      return ManualAction::TurnOff;
    }

    Serial.println("[CONTROL] STOP NC abierto con motor detenido: sin accion");
    return ManualAction::None;
  }

  startButtonDebouncer.update(isStartButtonPressed(), now);

  if (!startButtonDebouncer.rose()) {
    return ManualAction::None;
  }

  Serial.println("[CONTROL] START NO detectado");

  if (emergencyStopLatched) {
    Serial.println("[CONTROL] START ignorado: parada de emergencia activa");
    return ManualAction::None;
  }

  if (protectionActive) {
    Serial.println("[CONTROL] START ignorado: salir de PROTECCION requiere boton NC");
    return ManualAction::None;
  }

  Serial.print("[CONTROL] START conmuta relay a ");
  Serial.println(estadoSistema ? "OFF" : "ON");
  return estadoSistema ? ManualAction::TurnOff : ManualAction::TurnOn;
}

void updateLedIndicators(const uint8_t fsmState, const bool motorActive, const bool emergencyStopActive) {
  // Parada de emergencia: rojo encendido fijo hasta un segundo toque del NC.
  if (emergencyStopActive) {
    digitalWrite(Config::PIN_LED_RED, HIGH);
    digitalWrite(Config::PIN_LED_GREEN, LOW);
    return;
  }

  const unsigned long now = millis();
  uint32_t blinkInterval = Config::LED_BLINK_INTERVAL_NORMAL_MS;

  // Determinar intervalo de parpadeo según estado
  if (fsmState == static_cast<uint8_t>(FSM::State::SOBRECARGA)) {
    blinkInterval = Config::LED_BLINK_INTERVAL_OVERLOAD_MS;
  } else if (fsmState == static_cast<uint8_t>(FSM::State::PROTECCION)) {
    blinkInterval = Config::LED_BLINK_INTERVAL_PROTECTION_MS;
  }

  // Actualizar estado de parpadeo
  if (now - lastLedUpdateMs >= blinkInterval) {
    lastLedUpdateMs = now;
    ledBlinkState = !ledBlinkState;
  }

  if (fsmState == static_cast<uint8_t>(FSM::State::NORMAL) && motorActive) {
    // Estado NORMAL con motor activo: verde encendido fijo, rojo apagado
    digitalWrite(Config::PIN_LED_GREEN, HIGH);
    digitalWrite(Config::PIN_LED_RED, LOW);
  } else if (fsmState == static_cast<uint8_t>(FSM::State::SOBRECARGA)) {
    // SOBRECARGA: ambos parpadean
    digitalWrite(Config::PIN_LED_GREEN, ledBlinkState ? HIGH : LOW);
    digitalWrite(Config::PIN_LED_RED, ledBlinkState ? HIGH : LOW);
  } else if (fsmState == static_cast<uint8_t>(FSM::State::PROTECCION)) {
    // PROTECCION: ambos parpadean (con frecuencia distinta)
    digitalWrite(Config::PIN_LED_GREEN, ledBlinkState ? HIGH : LOW);
    digitalWrite(Config::PIN_LED_RED, ledBlinkState ? HIGH : LOW);
  } else {
    // Otros estados: LEDs apagados
    digitalWrite(Config::PIN_LED_GREEN, LOW);
    digitalWrite(Config::PIN_LED_RED, LOW);
  }
}

}  // namespace Control
