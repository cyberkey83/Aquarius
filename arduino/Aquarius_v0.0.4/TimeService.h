#pragma once

#include <Arduino.h>
#include <WiFi.h>

struct ClockReading {
  bool valid = false;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int day = 0;
  int month = 0;
  int year = 0;

  char timeText[9] = "--:--:--";
  char dateText[32] = "Time not synchronised";
};

class TimeService {
 public:
  void begin();
  void update();

  bool connectWiFi();
  void maintainWiFi();

  bool wifiConnected() const;
  bool timeValid() const;
  const ClockReading& reading() const { return reading_; }

  const char* wifiStatusText() const;

 private:
  void configureNtp();
  bool refreshLocalTime();

  ClockReading reading_;
  unsigned long lastWifiRetryMs_ = 0;
  bool ntpConfigured_ = false;
};
