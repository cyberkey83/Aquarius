#pragma once

#include <Arduino.h>
#include "AquariusConfig.h"

enum class PressureTrendDirection : uint8_t {
  Learning,
  Falling,
  Steady,
  Rising
};

class PressureTrend {
 public:
  void begin(float pressureHpa);
  void update(float pressureHpa);

  PressureTrendDirection direction() const;
  float changeHpa() const;
  const char* label() const;
  const char* symbol() const;

  uint8_t sampleCount() const { return count_; }

 private:
  void addSample(float pressureHpa);

  float samples_[AQUARIUS_PRESSURE_HISTORY_SIZE] = {};
  uint8_t count_ = 0;
  uint8_t nextIndex_ = 0;
  unsigned long lastSampleMs_ = 0;
};
