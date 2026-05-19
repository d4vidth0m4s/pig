#pragma once

#include <Arduino.h>

namespace Utils {

template <typename T>
constexpr T clamp(const T value, const T minValue, const T maxValue) {
  return (value < minValue) ? minValue : (value > maxValue ? maxValue : value);
}

class Debouncer {
 public:
  explicit Debouncer(uint32_t debounceMs = 50);

  bool update(bool rawState, uint32_t nowMs);
  bool rose() const;
  bool fell() const;
  bool state() const;

 private:
  uint32_t debounceMs_;
  uint32_t lastTransitionMs_;
  bool stableState_;
  bool lastRawState_;
  bool roseEvent_;
  bool fellEvent_;
};

}  // namespace Utils
