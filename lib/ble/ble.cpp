#include "ble.h"

#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_mac.h>

#include "../../include/config.h"
#include "../config/config_storage.h"

namespace {

BLEServer* bleServer = nullptr;
BLECharacteristic* txCharacteristic = nullptr;
BLECharacteristic* rxCharacteristic = nullptr;

bool deviceConnected = false;
BLEComms::Command pendingCommand = BLEComms::Command::None;

constexpr float kCurrentOverloadHysteresisA = 0.24f;
constexpr float kTemperatureOverloadHysteresisC = 10.0f;

bool hasNumericField(const JsonDocument& doc, const char* key) {
  return !doc[key].isNull();
}

float readFloatOrDefault(const JsonDocument& doc, const char* key, const float fallback) {
  return hasNumericField(doc, key) ? doc[key].as<float>() : fallback;
}

String formatMacAddress(const uint8_t mac[6]) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buffer);
}

void printBleConfiguration() {
  uint8_t bleMac[6];
  esp_read_mac(bleMac, ESP_MAC_BT);

  Serial.println("[BLE] ===== Identidad BLE =====");
  Serial.print("[BLE] deviceName: ");
  Serial.println(BLE_DEVICE_NAME);
  Serial.print("[BLE] deviceId / MAC: ");
  Serial.println(formatMacAddress(bleMac));
  Serial.print("[BLE] serviceUUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("[BLE] txCharacteristicUUID: ");
  Serial.println(CHARACTERISTIC_UUID_TX);
  Serial.print("[BLE] rxCharacteristicUUID: ");
  Serial.println(CHARACTERISTIC_UUID_RX);
  Serial.println("[BLE] =========================");
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    deviceConnected = true;
    Serial.println("[BLE] Cliente conectado");
  }

  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    Serial.println("[BLE] Cliente desconectado");
    server->getAdvertising()->start();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const std::string payload = characteristic->getValue();
    if (payload.empty()) {
      return;
    }

    Serial.print("[BLE] RX raw: ");
    Serial.println(payload.c_str());

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, payload.c_str());
    if (error) {
      Serial.print("[BLE] Error JSON RX: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc["comando"].is<String>()) {
      const String comando = doc["comando"].as<String>();

      if (comando == "config") {
        const bool hasCurrentFields = hasNumericField(doc, "I_nominal") &&
                                      hasNumericField(doc, "I_umbral1") &&
                                      hasNumericField(doc, "I_umbral2");
        const bool hasTemperatureFields =
            hasNumericField(doc, "T_umbral1") &&
            (hasNumericField(doc, "T_umbral2") || hasNumericField(doc, "T_max"));

        if (!hasCurrentFields || !hasTemperatureFields) {
          Serial.println("[BLE] Payload de config incompleto o invalido");
          return;
        }

        ConfigStorage::ControlConfig newConfig;
        newConfig.I_nominal = doc["I_nominal"].as<float>();
        newConfig.I_umbral1 = doc["I_umbral1"].as<float>();
        newConfig.I_umbral2 = doc["I_umbral2"].as<float>();
        newConfig.I_umbral1_des = readFloatOrDefault(
            doc, "I_umbral1_des", newConfig.I_umbral1 - kCurrentOverloadHysteresisA);
        newConfig.I_umbral2_arranque =
            readFloatOrDefault(doc, "I_umbral2_arranque", Config::I_umbral2_arranque);
        newConfig.T_umbral1 = doc["T_umbral1"].as<float>();
        newConfig.T_umbral1_des = readFloatOrDefault(
            doc, "T_umbral1_des", newConfig.T_umbral1 - kTemperatureOverloadHysteresisC);
        newConfig.T_max = hasNumericField(doc, "T_max")
                              ? doc["T_max"].as<float>()
                              : doc["T_umbral2"].as<float>();

        Serial.println("[BLE] Comando recibido: config");
        if (hasNumericField(doc, "T_nominal")) {
          Serial.print("[BLE] T_nominal recibido (informativo): ");
          Serial.println(doc["T_nominal"].as<float>(), 2);
        }
        if (hasNumericField(doc, "T_umbral2") && hasNumericField(doc, "T_max")) {
          const float tUmbral2 = doc["T_umbral2"].as<float>();
          const float tMax = doc["T_max"].as<float>();
          if (fabsf(tUmbral2 - tMax) > 0.01f) {
            Serial.print("[BLE] Aviso: T_umbral2=");
            Serial.print(tUmbral2, 2);
            Serial.print(" difiere de T_max=");
            Serial.println(tMax, 2);
          }
        }
        ConfigStorage::saveConfig(newConfig);
        return;
      }
    }

    if (doc["estado"].is<String>()) {
      const String comando = doc["estado"].as<String>();

      if (comando == "encender") {
        pendingCommand = BLEComms::Command::TurnOn;
        Serial.println("[BLE] Comando recibido: encender");
      } else if (comando == "apagar") {
        pendingCommand = BLEComms::Command::TurnOff;
        Serial.println("[BLE] Comando recibido: apagar");
      } else {
        Serial.print("[BLE] Comando no reconocido: ");
        Serial.println(comando);
      }
    } else {
      Serial.println("[BLE] Campo 'estado' ausente o invalido");
    }
  }
};

}  // namespace

namespace BLEComms {

void begin() {
  BLEDevice::init(BLE_DEVICE_NAME);
  printBleConfiguration();

  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  BLEService* service = bleServer->createService(SERVICE_UUID);

  txCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  txCharacteristic->addDescriptor(new BLE2902());

  rxCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
  rxCharacteristic->setCallbacks(new RxCallbacks());

  service->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  pAdvertising->setMaxPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Inicializado");
  Serial.println("[BLE] ✅ Publicidad BLE iniciada - esperando cliente...");
}

bool isConnected() { return deviceConnected; }

Command consumeCommand() {
  const Command command = pendingCommand;
  pendingCommand = Command::None;
  return command;
}

void sendTelemetry(const Telemetry& telemetry) {
  if (!deviceConnected || txCharacteristic == nullptr) {
    return;
  }

  JsonDocument doc;
  doc["estado"] = telemetry.estadoSistema;
  doc["rpm"] = telemetry.rpm;
  doc["temperatura"] = telemetry.temperatura;
  doc["corriente"] = telemetry.corriente;
  doc["timeon"] = telemetry.timeOn;
  doc["alertas"] = static_cast<uint32_t>(telemetry.alertas);

  String jsonString;
  serializeJson(doc, jsonString);

  txCharacteristic->setValue(jsonString.c_str());
  txCharacteristic->notify();
  Serial.print("[BLE] TX notify: ");
  Serial.println(jsonString.c_str());

}

}  // namespace BLEComms
