#pragma once

#include <Arduino.h>
#include "AquariusConfig.h"

class PressureTrend {
 public:
  void begin(float pressureHpa);
  void update(float pressureHpa);
  const char* label() const;
  const char* symbol() const;
  float changeHpa() const { return changeHpa_; }
  uint8_t sampleCount() const { return count_; }

 private:
  float history_[AQUARIUS_PRESSURE_HISTORY_SIZE] = {};
  uint8_t count_ = 0;
  uint8_t nextIndex_ = 0;
  float changeHpa_ = 0.0F;
  unsigned long lastSampleMs_ = 0;
  void addSample(float pressureHpa);
};
