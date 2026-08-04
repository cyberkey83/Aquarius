#pragma once

#include <Arduino.h>

struct ClockReading {
  String timeText = "--:--";
  String dateText = "Waiting for time";
  bool valid = false;
};

class TimeService {
 public:
  void begin(void (*progressCallback)() = nullptr);
  void update();
  const ClockReading& reading() const { return reading_; }
  bool wifiConnected() const;
  bool timeValid() const { return reading_.valid; }
  void setWifiCredentials(const String& ssid, const String& password);
  bool connectSavedWifi(void (*progressCallback)() = nullptr);

 private:
  ClockReading reading_;
  unsigned long lastWifiAttemptMs_ = 0;
  String configuredSsid_;
  String configuredPassword_;
  void connectWifi(void (*progressCallback)() = nullptr);
  void updateClock();
};
