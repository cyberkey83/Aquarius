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

 private:
  ClockReading reading_;
  unsigned long lastWifiAttemptMs_ = 0;
  void connectWifi(void (*progressCallback)() = nullptr);
  void updateClock();
};
