#include "utils.h"

namespace Utils {

Debouncer::Debouncer(uint32_t debounceMs)
    : debounceMs_(debounceMs),
      lastTransitionMs_(0),
      stableState_(false),
      lastRawState_(false),
      roseEvent_(false),
      fellEvent_(false) {}

bool Debouncer::update(bool rawState, uint32_t nowMs) {
  roseEvent_ = false;
  fellEvent_ = false;

  if (rawState != lastRawState_) {
    lastRawState_ = rawState;
    lastTransitionMs_ = nowMs;
  }

  if ((nowMs - lastTransitionMs_) >= debounceMs_ && stableState_ != lastRawState_) {
    stableState_ = lastRawState_;
    roseEvent_ = stableState_;
    fellEvent_ = !stableState_;
    return true;
  }

  return false;
}

bool Debouncer::rose() const { return roseEvent_; }

bool Debouncer::fell() const { return fellEvent_; }

bool Debouncer::state() const { return stableState_; }

}  // namespace Utils
