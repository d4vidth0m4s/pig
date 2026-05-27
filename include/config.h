#pragma once

#include <Arduino.h>

#define SERVICE_UUID "12345678-1234-5678-9abc-def123456789"
#define CHARACTERISTIC_UUID_TX "87654321-4321-8765-cba9-fed987654321"
#define CHARACTERISTIC_UUID_RX "11111111-2222-3333-4444-555555555555"
constexpr const char* BLE_DEVICE_NAME = "ESP32-Motor-Control";

namespace Config {

constexpr uint32_t SERIAL_BAUDRATE = 115200;

constexpr uint8_t PIN_RELAY = 33;
constexpr uint8_t PIN_NTC_10K = 34;
constexpr uint8_t PIN_ACS712 = 35;
constexpr uint8_t PIN_SENSOR_RPM = 32;
constexpr uint8_t PIN_START_BUTTON_NO = 27;
constexpr uint8_t PIN_STOP_BUTTON_NC = 26;
constexpr uint8_t BUZZER_PWM_CHANNEL = 0;
constexpr uint8_t PIN_LED_GREEN = 22;
constexpr uint8_t PIN_LED_RED = 19;

constexpr uint8_t PIN_BUZZER = 25;
constexpr uint8_t RELAY_ACTIVE_LEVEL = LOW;
constexpr uint8_t RELAY_INACTIVE_LEVEL = HIGH;

constexpr float I_NORMAL_MIN = 0.59f;
constexpr float I_nominal = 0.90f;
constexpr float I_umbral1 = 1.30f;
constexpr float I_umbral1_des = 1.06f;
constexpr float I_umbral2 = 1.50f;
constexpr float I_umbral2_arranque = 3.00f;

constexpr float T_umbral1 = 95.0f;
constexpr float T_umbral1_des = 85.0f;
constexpr float T_max = 105.0f;

constexpr uint32_t RPM_ALERT_THRESHOLD = 100;
constexpr float RPM_ALERT_CURRENT_THRESHOLD = 1.0f;
constexpr uint32_t RPM_PROTECTION_THRESHOLD = 0;
constexpr float RPM_PROTECTION_CURRENT_THRESHOLD = 1.50f;

constexpr uint32_t STARTUP_INHIBIT_WINDOW_MS = 5000;

constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;
constexpr float ADC_MAX_READING = 4095.0f;

constexpr float NTC_NOMINAL_RESISTANCE = 7740.0f; // measured
constexpr float NTC_NOMINAL_TEMPERATURE = 25.0f;
constexpr float NTC_BETA = 3950.0f;
constexpr float NTC_SERIES_RESISTANCE = 7290.0f; // measured
constexpr uint8_t NTC_AVERAGE_SAMPLES = 16;

constexpr float ACS712_DIVIDER_FACTOR = 0.5f;
constexpr float ACS712_ZERO_OFFSET_V = 1.25f;
constexpr float ACS712_ZERO_OFFSET_COUNTS = 1552.0f;  // (1.25V / 3.3V) * 4095 = 1551.136
constexpr float ACS712_SENSITIVITY_V_PER_A =  0.066f ;
constexpr float ACS712_EFFECTIVE_SENSITIVITY_V_PER_A =
    ACS712_SENSITIVITY_V_PER_A * ACS712_DIVIDER_FACTOR;
constexpr float ACS712_CURRENT_SCALE_A_PER_COUNT =
    (ADC_REFERENCE_VOLTAGE / ADC_MAX_READING) / ACS712_EFFECTIVE_SENSITIVITY_V_PER_A;
constexpr uint32_t ACS712_SAMPLE_WINDOW_MS = 120;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 80;

constexpr uint32_t SENSOR_SAMPLE_INTERVAL_MS = 250;
constexpr uint32_t BLE_NOTIFY_INTERVAL_MS = 1000;

constexpr uint32_t LED_BLINK_INTERVAL_NORMAL_MS = 500;
constexpr uint32_t LED_BLINK_INTERVAL_OVERLOAD_MS = 250;
constexpr uint32_t LED_BLINK_INTERVAL_PROTECTION_MS = 100;


}  // namespace Config
