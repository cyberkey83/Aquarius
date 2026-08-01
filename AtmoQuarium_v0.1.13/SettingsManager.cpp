#include "SettingsManager.h"
#include "AquariusConfig.h"

void SettingsManager::begin() {
  prefs_.begin(AQUARIUS_SETTINGS_NAMESPACE, false);
  load();
}
void SettingsManager::load() {
  settings_.fishCount = constrain((int)prefs_.getUChar("fish", 4), 1, 10);
  settings_.bubbleLevel = constrain((int)prefs_.getUChar("bubble", 2), 0, 10);
  settings_.plantLevel = constrain((int)prefs_.getUChar("plants", 2), 0, 10);
  settings_.plantLength = constrain((int)prefs_.getUChar("plen", 7), 1, 15);
  settings_.animationLevel = constrain((int)prefs_.getUChar("anim", 2), 0, 3);
  settings_.eventFrequency = constrain((int)prefs_.getUChar("events", 2), 0, 3);
  settings_.weatherEffects = prefs_.getBool("weather", true);
  settings_.jellyfishEnabled = prefs_.getBool("jelly", true);
  settings_.brightness = prefs_.getUChar("bright", AQUARIUS_DEFAULT_BRIGHTNESS);
  settings_.nightBrightness = prefs_.getUChar("night", AQUARIUS_DEFAULT_NIGHT_BRIGHTNESS);
  settings_.dimMinutes = constrain((int)prefs_.getUChar("dim", AQUARIUS_DEFAULT_DIM_MINUTES), 0, 60);
  settings_.use24Hour = prefs_.getBool("clock24", true);
  settings_.fahrenheit = prefs_.getBool("fahren", false);
  const uint8_t savedSensor = prefs_.getUChar("sensorsec", 15);
  const uint8_t validIntervals[] = {1, 5, 10, 15, 30, 60};
  settings_.sensorIntervalSec = 15;
  for (uint8_t v : validIntervals) if (savedSensor == v) settings_.sensorIntervalSec = v;
  settings_.wifiSsid = prefs_.getString("wifi_ssid", "");
  settings_.wifiPassword = prefs_.getString("wifi_pass", "");
  settings_.temperatureOffsetC = constrain(prefs_.getFloat("toff", 0.0f), -10.0f, 10.0f);
  settings_.humidityOffsetPct = constrain((int)prefs_.getChar("hoff", 0), -10, 10);
  settings_.weatherLatitude = constrain(prefs_.getFloat("wlat", AQUARIUS_LATITUDE), -90.0f, 90.0f);
  settings_.weatherLongitude = constrain(prefs_.getFloat("wlon", AQUARIUS_LONGITUDE), -180.0f, 180.0f);
  settings_.locationName = prefs_.getString("locname", AQUARIUS_LOCATION_NAME);
}
void SettingsManager::save() {
  prefs_.putUChar("fish", settings_.fishCount);
  prefs_.putUChar("bubble", settings_.bubbleLevel);
  prefs_.putUChar("plants", settings_.plantLevel);
  prefs_.putUChar("plen", settings_.plantLength);
  prefs_.putUChar("anim", settings_.animationLevel);
  prefs_.putUChar("events", settings_.eventFrequency);
  prefs_.putBool("weather", settings_.weatherEffects);
  prefs_.putBool("jelly", settings_.jellyfishEnabled);
  prefs_.putUChar("bright", settings_.brightness);
  prefs_.putUChar("night", settings_.nightBrightness);
  prefs_.putUChar("dim", settings_.dimMinutes);
  prefs_.putBool("clock24", settings_.use24Hour);
  prefs_.putBool("fahren", settings_.fahrenheit);
  prefs_.putUChar("sensorsec", settings_.sensorIntervalSec);
  prefs_.putString("wifi_ssid", settings_.wifiSsid);
  prefs_.putString("wifi_pass", settings_.wifiPassword);
  prefs_.putFloat("toff", settings_.temperatureOffsetC);
  prefs_.putChar("hoff", settings_.humidityOffsetPct);
  prefs_.putFloat("wlat", settings_.weatherLatitude);
  prefs_.putFloat("wlon", settings_.weatherLongitude);
  prefs_.putString("locname", settings_.locationName);
}
void SettingsManager::resetDefaults() {
  const String keepSsid = settings_.wifiSsid;
  const String keepPass = settings_.wifiPassword;
  settings_ = AquariusSettings();
  settings_.wifiSsid = keepSsid;
  settings_.wifiPassword = keepPass;
  save();
}

void SettingsManager::clearWifiCredentials() {
  settings_.wifiSsid = "";
  settings_.wifiPassword = "";
  prefs_.remove("wifi_ssid");
  prefs_.remove("wifi_pass");
}
