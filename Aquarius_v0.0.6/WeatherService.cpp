#include "WeatherService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "AquariusConfig.h"

void WeatherService::begin() {
  if (WiFi.status() == WL_CONNECTED) {
    fetch();
  }
}

void WeatherService::update() {
  if (requestInProgress_ || WiFi.status() != WL_CONNECTED) {
    return;
  }

  const unsigned long now = millis();
  const unsigned long interval =
      reading_.valid
          ? AQUARIUS_WEATHER_UPDATE_INTERVAL_MS
          : AQUARIUS_WEATHER_RETRY_INTERVAL_MS;

  if (lastAttemptMs_ == 0 || now - lastAttemptMs_ >= interval) {
    fetch();
  }
}

bool WeatherService::fetch() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  requestInProgress_ = true;
  lastAttemptMs_ = millis();

  HTTPClient http;
  http.setConnectTimeout(AQUARIUS_WEATHER_HTTP_TIMEOUT_MS);
  http.setTimeout(AQUARIUS_WEATHER_HTTP_TIMEOUT_MS);

  const String url = buildUrl();

  Serial.println("Requesting Open-Meteo weather...");
  Serial.println(url);

  if (!http.begin(url)) {
    Serial.println("Open-Meteo: HTTP client failed to start.");
    requestInProgress_ = false;
    return false;
  }

  const int responseCode = http.GET();

  if (responseCode != HTTP_CODE_OK) {
    Serial.printf("Open-Meteo HTTP error: %d\n", responseCode);
    http.end();
    requestInProgress_ = false;
    return false;
  }

  const String payload = http.getString();
  http.end();

  const bool parsed = parseResponse(payload);
  requestInProgress_ = false;

  if (parsed) {
    Serial.println("Open-Meteo weather updated.");
  } else {
    Serial.println("Open-Meteo response could not be parsed.");
  }

  return parsed;
}

String WeatherService::buildUrl() const {
  String url = "http://api.open-meteo.com/v1/forecast";
  url += "?latitude=";
  url += String(AQUARIUS_LATITUDE, 4);
  url += "&longitude=";
  url += String(AQUARIUS_LONGITUDE, 4);
  url += "&current=temperature_2m,relative_humidity_2m,";
  url += "apparent_temperature,weather_code,is_day,wind_speed_10m";
  url += "&temperature_unit=celsius";
  url += "&wind_speed_unit=kmh";
  url += "&daily=sunrise,sunset";
  url += "&timezone=auto";
  url += "&forecast_days=1";
  return url;
}

bool WeatherService::parseResponse(const String& payload) {
  JsonDocument document;
  const DeserializationError error = deserializeJson(document, payload);

  if (error) {
    Serial.printf("JSON error: %s\n", error.c_str());
    return false;
  }

  JsonObject current = document["current"];

  if (current.isNull()) {
    return false;
  }

  const float temperature = current["temperature_2m"] | NAN;
  const float humidity = current["relative_humidity_2m"] | NAN;
  const float apparent = current["apparent_temperature"] | NAN;
  const float wind = current["wind_speed_10m"] | NAN;
  const int code = current["weather_code"] | -1;
  const int dayFlag = current["is_day"] | 1;

  if (isnan(temperature) || isnan(humidity) || code < 0) {
    return false;
  }

  reading_.temperatureC = temperature;
  reading_.humidityPct = humidity;
  reading_.apparentTemperatureC = apparent;
  reading_.windSpeedKmh = wind;
  reading_.weatherCode = code;
  reading_.isDay = dayFlag == 1;
  reading_.valid = true;
  reading_.updatedAtMs = millis();

  strlcpy(
      reading_.conditionText,
      describeWeatherCode(code),
      sizeof(reading_.conditionText));

  const char* sunrise = document["daily"]["sunrise"][0] | "";
  const char* sunset = document["daily"]["sunset"][0] | "";

  if (strlen(sunrise) >= 16) {
    strlcpy(reading_.sunriseText, sunrise + 11, sizeof(reading_.sunriseText));
  }
  if (strlen(sunset) >= 16) {
    strlcpy(reading_.sunsetText, sunset + 11, sizeof(reading_.sunsetText));
  }

  return true;
}

bool WeatherService::isStale() const {
  if (!reading_.valid) {
    return true;
  }

  return millis() - reading_.updatedAtMs >
         (AQUARIUS_WEATHER_UPDATE_INTERVAL_MS * 2UL);
}

const char* WeatherService::statusText() const {
  if (!reading_.valid) {
    return "NO WEATHER";
  }

  return isStale() ? "WEATHER OLD" : "WEATHER OK";
}

const char* WeatherService::describeWeatherCode(int code) const {
  if (code == 0) return "CLEAR";
  if (code == 1) return "MAINLY CLEAR";
  if (code == 2) return "PARTLY CLOUDY";
  if (code == 3) return "OVERCAST";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 57) return "DRIZZLE";
  if (code >= 61 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "SHOWERS";
  if (code == 85 || code == 86) return "SNOW SHOWERS";
  if (code >= 95 && code <= 99) return "THUNDERSTORM";
  return "UNKNOWN";
}
