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
  settings_.plantLength = constrain((int)prefs_.getUChar("plen", 5), 1, 10);
  settings_.animationLevel = constrain((int)prefs_.getUChar("anim", 2), 0, 3);
  settings_.eventFrequency = constrain((int)prefs_.getUChar("events", 2), 0, 3);
  settings_.weatherEffects = prefs_.getBool("weather", true);
  settings_.brightness = prefs_.getUChar("bright", AQUARIUS_DEFAULT_BRIGHTNESS);
  settings_.nightBrightness = prefs_.getUChar("night", AQUARIUS_DEFAULT_NIGHT_BRIGHTNESS);
  settings_.dimMinutes = constrain((int)prefs_.getUChar("dim", AQUARIUS_DEFAULT_DIM_MINUTES), 0, 60);
  settings_.use24Hour = prefs_.getBool("clock24", true);
  settings_.fahrenheit = prefs_.getBool("fahren", false);
}
void SettingsManager::save() {
  prefs_.putUChar("fish", settings_.fishCount);
  prefs_.putUChar("bubble", settings_.bubbleLevel);
  prefs_.putUChar("plants", settings_.plantLevel);
  prefs_.putUChar("plen", settings_.plantLength);
  prefs_.putUChar("anim", settings_.animationLevel);
  prefs_.putUChar("events", settings_.eventFrequency);
  prefs_.putBool("weather", settings_.weatherEffects);
  prefs_.putUChar("bright", settings_.brightness);
  prefs_.putUChar("night", settings_.nightBrightness);
  prefs_.putUChar("dim", settings_.dimMinutes);
  prefs_.putBool("clock24", settings_.use24Hour);
  prefs_.putBool("fahren", settings_.fahrenheit);
}
void SettingsManager::resetDefaults() { settings_ = AquariusSettings(); save(); }
