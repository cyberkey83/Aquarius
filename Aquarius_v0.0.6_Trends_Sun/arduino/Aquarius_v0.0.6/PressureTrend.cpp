#include "PressureTrend.h"

void PressureTrend::begin(float pressureHpa) {
  if (!isnan(pressureHpa)) {
    addSample(pressureHpa);
  }
}

void PressureTrend::update(float pressureHpa) {
  if (isnan(pressureHpa)) return;

  const unsigned long now = millis();
  if (count_ == 0 ||
      now - lastSampleMs_ >= AQUARIUS_PRESSURE_SAMPLE_INTERVAL_MS) {
    addSample(pressureHpa);
  }
}

void PressureTrend::addSample(float pressureHpa) {
  samples_[nextIndex_] = pressureHpa;
  nextIndex_ = (nextIndex_ + 1) % AQUARIUS_PRESSURE_HISTORY_SIZE;
  if (count_ < AQUARIUS_PRESSURE_HISTORY_SIZE) count_++;
  lastSampleMs_ = millis();
}

float PressureTrend::changeHpa() const {
  if (count_ < 2) return 0.0F;

  const uint8_t oldestIndex =
      count_ < AQUARIUS_PRESSURE_HISTORY_SIZE ? 0 : nextIndex_;
  const uint8_t newestIndex =
      (nextIndex_ + AQUARIUS_PRESSURE_HISTORY_SIZE - 1) %
      AQUARIUS_PRESSURE_HISTORY_SIZE;

  return samples_[newestIndex] - samples_[oldestIndex];
}

PressureTrendDirection PressureTrend::direction() const {
  if (count_ < 3) return PressureTrendDirection::Learning;

  const float delta = changeHpa();
  if (delta > AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA)
    return PressureTrendDirection::Rising;
  if (delta < -AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA)
    return PressureTrendDirection::Falling;
  return PressureTrendDirection::Steady;
}

const char* PressureTrend::label() const {
  switch (direction()) {
    case PressureTrendDirection::Rising: return "RISING";
    case PressureTrendDirection::Falling: return "FALLING";
    case PressureTrendDirection::Steady: return "STEADY";
    default: return "LEARNING";
  }
}

const char* PressureTrend::symbol() const {
  switch (direction()) {
    case PressureTrendDirection::Rising: return "^";
    case PressureTrendDirection::Falling: return "v";
    case PressureTrendDirection::Steady: return "-";
    default: return "?";
  }
}
