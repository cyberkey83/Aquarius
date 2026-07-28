#include "TimeService.h"
#include <WiFi.h>
#include <time.h>
#include "AquariusConfig.h"
#include "Secrets.h"

void TimeService::connectWifi(void (*progressCallback)()) {
  if (WiFi.status() == WL_CONNECTED) return;
  lastWifiAttemptMs_ = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(AQUARIUS_WIFI_SSID, AQUARIUS_WIFI_PASSWORD);
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < AQUARIUS_WIFI_CONNECT_TIMEOUT_MS) {
    if (progressCallback != nullptr) progressCallback();
    delay(60);
  }
}

void TimeService::begin(void (*progressCallback)()) {
  connectWifi(progressCallback);
  configTzTime(AQUARIUS_TZ, AQUARIUS_NTP_1, AQUARIUS_NTP_2, AQUARIUS_NTP_3);
  updateClock();
}

void TimeService::updateClock() {
  struct tm info;
  if (!getLocalTime(&info, 20)) {
    reading_.valid = false;
    reading_.timeText = "--:--";
    reading_.dateText = "Waiting for time";
    return;
  }
  char timeBuffer[9];
  char dateBuffer[32];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &info);
  strftime(dateBuffer, sizeof(dateBuffer), "%A %d %B", &info);
  reading_.timeText = timeBuffer;
  reading_.dateText = dateBuffer;
  reading_.valid = true;
}

void TimeService::update() {
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiAttemptMs_ >= AQUARIUS_WIFI_RETRY_INTERVAL_MS) connectWifi(nullptr);
  updateClock();
}

bool TimeService::wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
