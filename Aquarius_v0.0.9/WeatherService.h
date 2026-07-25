#pragma once

#include <Arduino.h>

struct OutdoorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float windSpeedKmh = NAN;
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
  const OutdoorReading& reading() const { return reading_; }

 private:
  OutdoorReading reading_;
  unsigned long lastAttemptMs_ = 0;
  unsigned long lastSuccessMs_ = 0;
  bool fetch();
  const char* conditionForCode(int code) const;
  void copyClockText(const char* isoText, char output[6]);
};
