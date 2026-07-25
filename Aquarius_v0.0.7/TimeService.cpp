#include "TimeService.h"

#include <time.h>
#include "AquariusConfig.h"
#include "Secrets.h"

void TimeService::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  connectWiFi();

  if (wifiConnected()) {
    configureNtp();
  }

  refreshLocalTime();
}

bool TimeService::connectWiFi() {
  if (wifiConnected()) {
    return true;
  }

  Serial.printf(
      "Connecting to Wi-Fi: %s\n",
      AQUARIUS_WIFI_SSID);

  WiFi.begin(
      AQUARIUS_WIFI_SSID,
      AQUARIUS_WIFI_PASSWORD);

  const unsigned long start = millis();

  while (!wifiConnected() &&
         millis() - start < AQUARIUS_WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }

  Serial.println();

  if (wifiConnected()) {
    Serial.printf(
        "Wi-Fi connected. IP: %s\n",
        WiFi.localIP().toString().c_str());

    if (!ntpConfigured_) {
      configureNtp();
    }

    return true;
  }

  Serial.println("Wi-Fi connection timed out.");
  return false;
}

void TimeService::maintainWiFi() {
  if (wifiConnected()) {
    return;
  }

  const unsigned long now = millis();

  if (now - lastWifiRetryMs_ >=
      AQUARIUS_WIFI_RETRY_INTERVAL_MS) {
    lastWifiRetryMs_ = now;
    connectWiFi();
  }
}

void TimeService::configureNtp() {
  /*
   * configTzTime configures SNTP and the local POSIX timezone.
   * AQUARIUS_TZ handles GMT/BST automatically.
   */
  configTzTime(
      AQUARIUS_TZ,
      AQUARIUS_NTP_1,
      AQUARIUS_NTP_2,
      AQUARIUS_NTP_3);

  ntpConfigured_ = true;

  Serial.printf(
      "NTP configured. TZ=%s\n",
      AQUARIUS_TZ);
}

void TimeService::update() {
  maintainWiFi();
  refreshLocalTime();
}

bool TimeService::refreshLocalTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 50)) {
    reading_.valid = false;

    strlcpy(
        reading_.timeText,
        "--:--:--",
        sizeof(reading_.timeText));

    strlcpy(
        reading_.dateText,
        "Waiting for NTP...",
        sizeof(reading_.dateText));

    return false;
  }

  // Reject obviously-unsynchronised epoch-derived dates.
  if (timeinfo.tm_year + 1900 < 2025) {
    reading_.valid = false;
    return false;
  }

  reading_.valid = true;
  reading_.hour = timeinfo.tm_hour;
  reading_.minute = timeinfo.tm_min;
  reading_.second = timeinfo.tm_sec;
  reading_.day = timeinfo.tm_mday;
  reading_.month = timeinfo.tm_mon + 1;
  reading_.year = timeinfo.tm_year + 1900;

  strftime(
      reading_.timeText,
      sizeof(reading_.timeText),
      "%H:%M:%S",
      &timeinfo);

  strftime(
      reading_.dateText,
      sizeof(reading_.dateText),
      "%a %d %b %Y",
      &timeinfo);

  return true;
}

bool TimeService::wifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool TimeService::timeValid() const {
  return reading_.valid;
}

const char* TimeService::wifiStatusText() const {
  return wifiConnected() ? "WIFI OK" : "WIFI --";
}
