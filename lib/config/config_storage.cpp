#include "config_storage.h"

#include <Preferences.h>

#include "../../include/config.h"

namespace {

constexpr const char* kNamespace = "control";
constexpr uint32_t kConfigVersion = 2;
constexpr const char* kKeyVersion = "cfg_ver";
constexpr const char* kKeyINominal = "I_nom";
constexpr const char* kKeyIUmbral1 = "I_umb1";
constexpr const char* kKeyIUmbral1Des = "I_umb1_d";
constexpr const char* kKeyIUmbral2 = "I_umb2";
constexpr const char* kKeyIUmbral2Arranque = "I_u2_arr";
constexpr const char* kKeyTUmbral1 = "T_umb1";
constexpr const char* kKeyTUmbral1Des = "T_umb1_d";
constexpr const char* kKeyTMax = "T_max";

Preferences preferences;
ConfigStorage::ControlConfig activeConfig;

ConfigStorage::ControlConfig defaultConfig() {
  ConfigStorage::ControlConfig config;
  config.I_nominal = Config::I_nominal;
  config.I_umbral1 = Config::I_umbral1;
  config.I_umbral1_des = Config::I_umbral1_des;
  config.I_umbral2 = Config::I_umbral2;
  config.I_umbral2_arranque = Config::I_umbral2_arranque;
  config.T_umbral1 = Config::T_umbral1;
  config.T_umbral1_des = Config::T_umbral1_des;
  config.T_max = Config::T_max;
  return config;
}

ConfigStorage::ControlConfig sanitizeConfig(
    const ConfigStorage::ControlConfig& candidate) {
  const ConfigStorage::ControlConfig defaults = defaultConfig();
  ConfigStorage::ControlConfig sanitized = candidate;

  if (sanitized.I_nominal <= 0.0f) {
    sanitized.I_nominal = defaults.I_nominal;
  }

  if (sanitized.I_umbral1 < sanitized.I_nominal) {
    sanitized.I_umbral1 = defaults.I_umbral1 >= sanitized.I_nominal
                              ? defaults.I_umbral1
                              : sanitized.I_nominal;
  }

  if (sanitized.I_umbral2 < sanitized.I_umbral1) {
    sanitized.I_umbral2 = defaults.I_umbral2 >= sanitized.I_umbral1
                              ? defaults.I_umbral2
                              : sanitized.I_umbral1;
  }

  if (sanitized.I_umbral1_des <= 0.0f || sanitized.I_umbral1_des >= sanitized.I_umbral1) {
    sanitized.I_umbral1_des = defaults.I_umbral1_des < sanitized.I_umbral1
                                   ? defaults.I_umbral1_des
                                   : sanitized.I_umbral1;
  }

  if (sanitized.I_umbral2_arranque < sanitized.I_umbral2) {
    sanitized.I_umbral2_arranque = defaults.I_umbral2_arranque >= sanitized.I_umbral2
                                       ? defaults.I_umbral2_arranque
                                       : sanitized.I_umbral2;
  }

  if (sanitized.T_umbral1 <= 0.0f) {
    sanitized.T_umbral1 = defaults.T_umbral1;
  }

  if (sanitized.T_umbral1_des <= 0.0f || sanitized.T_umbral1_des >= sanitized.T_umbral1) {
    sanitized.T_umbral1_des = defaults.T_umbral1_des < sanitized.T_umbral1
                                  ? defaults.T_umbral1_des
                                  : sanitized.T_umbral1;
  }

  if (sanitized.T_max < sanitized.T_umbral1) {
    sanitized.T_max = defaults.T_max;
  }

  return sanitized;
}

void logConfig(const char* prefix, const ConfigStorage::ControlConfig& config) {
  Serial.print(prefix);
  Serial.print(" I_nominal=");
  Serial.print(config.I_nominal, 2);
  Serial.print(" I_umbral1=");
  Serial.print(config.I_umbral1, 2);
  Serial.print(" I_umbral1_des=");
  Serial.print(config.I_umbral1_des, 2);
  Serial.print(" I_umbral2=");
  Serial.print(config.I_umbral2, 2);
  Serial.print(" I_umbral2_arranque=");
  Serial.print(config.I_umbral2_arranque, 2);
  Serial.print(" T_umbral1=");
  Serial.print(config.T_umbral1, 2);
  Serial.print(" T_umbral1_des=");
  Serial.print(config.T_umbral1_des, 2);
  Serial.print(" T_max=");
  Serial.println(config.T_max, 2);
}

}  // namespace

namespace ConfigStorage {

void begin() {
  activeConfig = defaultConfig();
  Serial.println("[ConfigStorage] Inicializado");
}

ControlConfig loadConfig() {
  const ControlConfig defaults = defaultConfig();

  if (!preferences.begin(kNamespace, false)) {
    Serial.println("[ConfigStorage] No se pudo abrir Preferences, usando defaults");
    activeConfig = defaults;
    return activeConfig;
  }

  const uint32_t storedVersion = preferences.getUInt(kKeyVersion, 0U);
  if (storedVersion != kConfigVersion) {
    Serial.println("[ConfigStorage] Version de config anterior detectada, aplicando nuevos defaults");
    preferences.clear();
    activeConfig = defaults;
    preferences.putUInt(kKeyVersion, kConfigVersion);
    preferences.putFloat(kKeyINominal, activeConfig.I_nominal);
    preferences.putFloat(kKeyIUmbral1, activeConfig.I_umbral1);
    preferences.putFloat(kKeyIUmbral1Des, activeConfig.I_umbral1_des);
    preferences.putFloat(kKeyIUmbral2, activeConfig.I_umbral2);
    preferences.putFloat(kKeyIUmbral2Arranque, activeConfig.I_umbral2_arranque);
    preferences.putFloat(kKeyTUmbral1, activeConfig.T_umbral1);
    preferences.putFloat(kKeyTUmbral1Des, activeConfig.T_umbral1_des);
    preferences.putFloat(kKeyTMax, activeConfig.T_max);
    preferences.end();
    logConfig("[ConfigStorage] Config migrada:", activeConfig);
    return activeConfig;
  }

  activeConfig.I_nominal = preferences.getFloat(kKeyINominal, defaults.I_nominal);
  activeConfig.I_umbral1 = preferences.getFloat(kKeyIUmbral1, defaults.I_umbral1);
  activeConfig.I_umbral1_des =
      preferences.getFloat(kKeyIUmbral1Des, defaults.I_umbral1_des);
  activeConfig.I_umbral2 = preferences.getFloat(kKeyIUmbral2, defaults.I_umbral2);
  activeConfig.I_umbral2_arranque =
      preferences.getFloat(kKeyIUmbral2Arranque, defaults.I_umbral2_arranque);
  activeConfig.T_umbral1 = preferences.getFloat(kKeyTUmbral1, defaults.T_umbral1);
  activeConfig.T_umbral1_des =
      preferences.getFloat(kKeyTUmbral1Des, defaults.T_umbral1_des);
  activeConfig.T_max = preferences.getFloat(kKeyTMax, defaults.T_max);

  activeConfig = sanitizeConfig(activeConfig);

  preferences.end();

  logConfig("[ConfigStorage] Config cargada:", activeConfig);
  return activeConfig;
}

const ControlConfig& currentConfig() { return activeConfig; }

void saveConfig(const ControlConfig& config) {
  const ControlConfig sanitizedConfig = sanitizeConfig(config);

  if (!preferences.begin(kNamespace, false)) {
    Serial.println("[ConfigStorage] No se pudo abrir Preferences para guardar");
    return;
  }

  preferences.putUInt(kKeyVersion, kConfigVersion);
  preferences.putFloat(kKeyINominal, sanitizedConfig.I_nominal);
  preferences.putFloat(kKeyIUmbral1, sanitizedConfig.I_umbral1);
  preferences.putFloat(kKeyIUmbral1Des, sanitizedConfig.I_umbral1_des);
  preferences.putFloat(kKeyIUmbral2, sanitizedConfig.I_umbral2);
  preferences.putFloat(kKeyIUmbral2Arranque, sanitizedConfig.I_umbral2_arranque);
  preferences.putFloat(kKeyTUmbral1, sanitizedConfig.T_umbral1);
  preferences.putFloat(kKeyTUmbral1Des, sanitizedConfig.T_umbral1_des);
  preferences.putFloat(kKeyTMax, sanitizedConfig.T_max);
  preferences.end();

  activeConfig = sanitizedConfig;
  logConfig("[ConfigStorage] Config guardada:", activeConfig);
}

void resetToDefaults() {
  const ControlConfig defaults = defaultConfig();

  if (!preferences.begin(kNamespace, false)) {
    Serial.println("[ConfigStorage] No se pudo abrir Preferences para reset");
    activeConfig = defaults;
    return;
  }

  preferences.clear();
  preferences.putUInt(kKeyVersion, kConfigVersion);
  preferences.end();

  activeConfig = defaults;
  logConfig("[ConfigStorage] Config reseteada:", activeConfig);
}

}  // namespace ConfigStorage
