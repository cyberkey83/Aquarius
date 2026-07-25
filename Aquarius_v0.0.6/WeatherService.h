#pragma once

#include <Arduino.h>

struct OutdoorReading {
  bool valid = false;

  float temperatureC = NAN;
  float humidityPct = NAN;
  float apparentTemperatureC = NAN;
  float windSpeedKmh = NAN;

  int weatherCode = -1;
  bool isDay = true;

  unsigned long updatedAtMs = 0;
  char conditionText[20] = "NO DATA";
  char sunriseText[6] = "--:--";
  char sunsetText[6] = "--:--";
};

class WeatherService {
 public:
  void begin();
  void update();

  bool fetch();
  bool hasValidData() const { return reading_.valid; }
  bool isStale() const;

  const OutdoorReading& reading() const { return reading_; }
  const char* statusText() const;

 private:
  String buildUrl() const;
  bool parseResponse(const String& payload);
  const char* describeWeatherCode(int code) const;

  OutdoorReading reading_;
  unsigned long lastAttemptMs_ = 0;
  bool requestInProgress_ = false;
};
