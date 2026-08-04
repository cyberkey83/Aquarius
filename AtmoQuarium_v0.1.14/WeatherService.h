#pragma once

#include <Arduino.h>
#include "AquariusConfig.h"

struct OutdoorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float windSpeedKmh = NAN;
  float windDirectionDeg = NAN;
  int weatherCode = -1;
  char conditionText[24] = "WAITING";
  char sunriseText[6] = "--:--";
  char sunsetText[6] = "--:--";
  bool valid = false;
};

class WeatherService {
 public:
  void begin();
  void update();
  void setLocation(float latitude, float longitude) { latitude_ = constrain(latitude, -90.0f, 90.0f); longitude_ = constrain(longitude, -180.0f, 180.0f); }
  void refreshNow() { lastAttemptMs_ = 0; reading_.valid = false; }
  const OutdoorReading& reading() const { return reading_; }
  unsigned long lastSuccessMs() const { return lastSuccessMs_; }

 private:
  OutdoorReading reading_;
  unsigned long lastAttemptMs_ = 0;
  unsigned long lastSuccessMs_ = 0;
  float latitude_ = AQUARIUS_LATITUDE;
  float longitude_ = AQUARIUS_LONGITUDE;
  bool fetch();
  const char* conditionForCode(int code) const;
  void copyClockText(const char* isoText, char output[6]);
};
