#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "TimeService.h"
#include "WeatherService.h"

class AmbientRenderer {
 public:
  explicit AmbientRenderer(TFT_eSprite& canvas);

  uint16_t backgroundColour(
      const ClockReading& clock,
      const OutdoorReading& outdoor) const;

  void draw(
      const ClockReading& clock,
      const OutdoorReading& outdoor,
      unsigned long nowMs,
      uint16_t background);

 private:
  TFT_eSprite& canvas_;

  bool isNight(const ClockReading& clock) const;
  void drawWaterSurface(unsigned long nowMs, uint16_t background);
  void drawBubbles(unsigned long nowMs, uint16_t background);
  void drawFish(unsigned long nowMs, uint16_t background);
  void drawPlants(unsigned long nowMs, uint16_t background);
};
