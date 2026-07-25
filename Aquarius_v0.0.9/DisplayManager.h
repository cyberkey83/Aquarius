#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AmbientRenderer.h"
#include "IndoorSensor.h"
#include "TimeService.h"
#include "WeatherService.h"

class DisplayManager {
 public:
  explicit DisplayManager(TFT_eSprite& canvas);

  void drawBootScreen();
  void drawBootLine(const String& text, int y, uint16_t colour);

  void drawDashboard(
      const ClockReading& clock,
      const IndoorReading& indoor,
      const OutdoorReading& outdoor,
      bool wifiConnected,
      const char* pressureTrendLabel,
      const char* pressureTrendSymbol,
      unsigned long nowMs);

 private:
  TFT_eSprite& canvas_;
  AmbientRenderer ambient_;
  uint16_t background_ = TFT_BLACK;

  void clear(const ClockReading* clock = nullptr, const OutdoorReading* outdoor = nullptr);
  void drawHeader(const ClockReading& clock, bool wifiConnected);
  void drawWeatherPanel(
      const IndoorReading& indoor,
      const OutdoorReading& outdoor);
  void drawFooter(
      const ClockReading& clock,
      const IndoorReading& indoor,
      const OutdoorReading& outdoor,
      const char* pressureTrendLabel,
      const char* pressureTrendSymbol,
      unsigned long nowMs);

  String temperatureText(float value, bool valid) const;
  String humidityText(float value, bool valid) const;
  String compactCondition(const char* condition) const;
};
