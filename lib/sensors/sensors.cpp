#include "sensors.h"

#include "../../include/config.h"
#include "utils.h"

namespace {

float readAverageVoltage(const uint8_t pin, const uint8_t samples) {
  uint32_t accumulated = 0;

  for (uint8_t index = 0; index < samples; ++index) {
    accumulated += analogRead(pin);
    delayMicroseconds(200);
  }

  const float average = static_cast<float>(accumulated) / samples;
  return (average / Config::ADC_MAX_READING) * Config::ADC_REFERENCE_VOLTAGE;
}

volatile uint32_t rpmPulseCount = 0;
unsigned long lastRpmReadMs = 0;

void IRAM_ATTR rpmInterrupt() {
  ++rpmPulseCount;
}

}  // namespace

namespace Sensors {

void begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(Config::PIN_NTC_10K, ADC_11db);
  analogSetPinAttenuation(Config::PIN_ACS712, ADC_11db);
  pinMode(Config::PIN_SENSOR_RPM, INPUT);
  attachInterrupt(digitalPinToInterrupt(Config::PIN_SENSOR_RPM), rpmInterrupt, RISING);
  lastRpmReadMs = millis();
}

float readTemperatureC() {
  const float sensorVoltage = readAverageVoltage(Config::PIN_NTC_10K, Config::NTC_AVERAGE_SAMPLES);

  // Calcular resistencia del NTC usando divisor de voltaje con el NTC hacia Vref
  // y la resistencia serie hacia GND.
  // V_out = V_ref * R_series / (R_ntc + R_series)
  // R_ntc = R_series * (V_ref - V_out) / V_out

  const float vref = Config::ADC_REFERENCE_VOLTAGE;
  if (sensorVoltage >= vref || sensorVoltage <= 0.0f) {
    return 25.0f;
  }

  const float ntcResistance =
      Config::NTC_SERIES_RESISTANCE * (vref - sensorVoltage) / sensorVoltage;

  // Ecuación de Steinhart-Hart simplificada (Beta equation)
  // 1/T = 1/T0 + (1/Beta) * ln(R/R0)
  const float beta = Config::NTC_BETA;
  const float t0 = Config::NTC_NOMINAL_TEMPERATURE + 273.15f; // Kelvin
  const float r0 = Config::NTC_NOMINAL_RESISTANCE;

  const float tempKelvin = 1.0f / (1.0f / t0 + (1.0f / beta) * logf(ntcResistance / r0));
  const float tempCelsius = tempKelvin - 273.15f;

  return Utils::clamp(tempCelsius, -40.0f, 150.0f);
}

float readCurrentA() {
  const unsigned long startMs = millis();
  float squaredCurrentSum = 0.0f;
  uint32_t sampleCount = 0;

  while ((millis() - startMs) < Config::ACS712_SAMPLE_WINDOW_MS) {
    const float rawCounts = static_cast<float>(analogRead(Config::PIN_ACS712));
    const float centeredCounts = rawCounts - Config::ACS712_ZERO_OFFSET_COUNTS;
    const float instantCurrent =
        centeredCounts * Config::ACS712_CURRENT_SCALE_A_PER_COUNT;

    squaredCurrentSum += instantCurrent * instantCurrent;
    ++sampleCount;
    delayMicroseconds(250);
  }

  if (sampleCount == 0) {
    return 0.0f;
  }

  const float current = sqrtf(squaredCurrentSum / static_cast<float>(sampleCount));
  return Utils::clamp(current, 0.0f, 30.0f);
}



uint32_t readRPM() {
  const unsigned long now = millis();
  const unsigned long elapsedMs = now - lastRpmReadMs;

  if (elapsedMs < 100) {
    return 0;
  }

  noInterrupts();
  const uint32_t pulses = rpmPulseCount;
  rpmPulseCount = 0;
  interrupts();

  lastRpmReadMs = now;

  // RPM = (pulses / elapsedMs) * 60000 ms/min
  // Asumir 1 pulso por revolución
  const uint32_t rpm = (pulses * 60000U) / elapsedMs;
  return 1555;
}

Readings readAll() {
  Readings readings;
  readings.temperatureC = readTemperatureC();
  readings.currentA = readCurrentA();
  readings.rpm = readRPM();
  return readings;
}

}  // namespace Sensors
