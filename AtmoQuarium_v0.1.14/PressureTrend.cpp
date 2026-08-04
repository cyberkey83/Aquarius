#include "PressureTrend.h"
#include <math.h>

void PressureTrend::begin(float pressureHpa) {
  count_ = 0;
  nextIndex_ = 0;
  changeHpa_ = 0.0F;
  lastSampleMs_ = millis();
  addSample(pressureHpa);
}

void PressureTrend::addSample(float pressureHpa) {
  if (isnan(pressureHpa)) return;
  history_[nextIndex_] = pressureHpa;
  nextIndex_ = (nextIndex_ + 1) % AQUARIUS_PRESSURE_HISTORY_SIZE;
  if (count_ < AQUARIUS_PRESSURE_HISTORY_SIZE) ++count_;

  if (count_ < 2) { changeHpa_ = 0.0F; return; }
  const uint8_t oldest = count_ < AQUARIUS_PRESSURE_HISTORY_SIZE ? 0 : nextIndex_;
  changeHpa_ = pressureHpa - history_[oldest];
}

void PressureTrend::update(float pressureHpa) {
  const unsigned long now = millis();
  if (count_ == 0) { begin(pressureHpa); return; }
  if (now - lastSampleMs_ < AQUARIUS_PRESSURE_SAMPLE_INTERVAL_MS) return;
  lastSampleMs_ = now;
  addSample(pressureHpa);
}

const char* PressureTrend::label() const {
  if (count_ < 2) return "LEARNING";
  if (changeHpa_ > AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA) return "RISING";
  if (changeHpa_ < -AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA) return "FALLING";
  return "STEADY";
}

const char* PressureTrend::symbol() const {
  if (count_ < 2) return "";
  if (changeHpa_ > AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA) return "+";
  if (changeHpa_ < -AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA) return "-";
  return "=";
}
