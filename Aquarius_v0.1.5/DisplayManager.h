#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AquariumEngine.h"
#include "IndoorSensor.h"
#include "TimeService.h"
#include "WeatherService.h"

class DisplayManager {
 public:
  explicit DisplayManager(TFT_eSprite& canvas);

  void beginAquarium(unsigned long nowMs);
  void updateAquarium(unsigned long nowMs, const ClockReading& clock, const OutdoorReading& outdoor);
  void feedFish(unsigned long nowMs, int screenX, int screenY);
  bool feedingActive(unsigned long nowMs) const;
  void configureDisplay(bool use24Hour, bool fahrenheit);
  void configureAquarium(uint8_t fishCount, uint8_t bubbleLevel, uint8_t plantLevel,
                         uint8_t animationLevel, bool weatherEffects, uint8_t eventFrequency);

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
  AquariumEngine aquarium_;
  uint16_t background_ = TFT_BLACK;
  bool use24Hour_ = true;
  bool fahrenheit_ = false;
  String clockText(const ClockReading& clock) const;

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
