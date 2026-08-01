#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct AquariusSettings {
  uint8_t fishCount = 4;
  uint8_t bubbleLevel = 2;      // 0..10
  uint8_t plantLevel = 2;       // 0..10
  uint8_t plantLength = 7;      // 1..15
  uint8_t animationLevel = 2;   // 0..3
  uint8_t eventFrequency = 2;   // 0..3
  bool weatherEffects = true;
  bool jellyfishEnabled = true;
  uint8_t brightness = 220;
  uint8_t nightBrightness = 90;
  uint8_t dimMinutes = 10;      // 0 = never
  bool use24Hour = true;
  bool fahrenheit = false;
  uint8_t sensorIntervalSec = 15; // 1,5,10,15,30,60
  String wifiSsid = "";
  String wifiPassword = "";
};

class SettingsManager {
 public:
  void begin();
  void save();
  void resetDefaults();
  void clearWifiCredentials();
  AquariusSettings& values() { return settings_; }
  const AquariusSettings& values() const { return settings_; }
 private:
  Preferences prefs_;
  AquariusSettings settings_;
  void load();
};
