#include "WeatherService.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "AquariusConfig.h"

const char* WeatherService::conditionForCode(int code) const {
  if (code == 0) return "CLEAR";
  if (code == 1) return "MAINLY CLEAR";
  if (code == 2) return "PARTLY CLOUDY";
  if (code == 3) return "OVERCAST";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 57) return "DRIZZLE";
  if (code >= 61 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "SHOWERS";
  if (code >= 85 && code <= 86) return "SNOW SHOWERS";
  if (code >= 95) return "THUNDERSTORM";
  return "UNKNOWN";
}

void WeatherService::copyClockText(const char* isoText, char output[6]) {
  strcpy(output, "--:--");
  if (!isoText) return;
  const char* t = strchr(isoText, 'T');
  if (t && strlen(t) >= 6) {
    memcpy(output, t + 1, 5);
    output[5] = '\0';
  }
}

void WeatherService::begin() { fetch(); }

void WeatherService::update() {
  const unsigned long now = millis();
  const unsigned long interval = reading_.valid ? AQUARIUS_WEATHER_UPDATE_INTERVAL_MS : AQUARIUS_WEATHER_RETRY_INTERVAL_MS;
  if (now - lastAttemptMs_ >= interval) fetch();
}

bool WeatherService::fetch() {
  lastAttemptMs_ = millis();
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(AQUARIUS_LATITUDE, 4);
  url += "&longitude=";
  url += String(AQUARIUS_LONGITUDE, 4);
  url += "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m";
  url += "&daily=sunrise,sunset&timezone=auto&forecast_days=1";

  WiFiClientSecure client;
  // Open-Meteo uses HTTPS. For this embedded dashboard we accept the
  // server certificate without storing a CA bundle on the ESP32.
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(AQUARIUS_WEATHER_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  Serial.print("Weather URL        : ");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println("Weather error      : HTTP begin failed");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error : %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return false;
  }

  // Read the complete response before parsing it. Parsing HTTPClient's live
  // stream can fail on ESP32 when the server uses chunked transfer encoding.
  const String payload = http.getString();
  http.end();

  Serial.printf("Weather response   : %u bytes\n", static_cast<unsigned>(payload.length()));

  if (payload.length() == 0) {
    Serial.println("Weather error      : empty response body");
    return false;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("Weather JSON error : %s\n", error.c_str());
    Serial.print("Weather body       : ");
    Serial.println(payload.substring(0, 240));
    return false;
  }

  OutdoorReading next;
  next.temperatureC = doc["current"]["temperature_2m"] | NAN;
  next.humidityPct = doc["current"]["relative_humidity_2m"] | NAN;
  next.weatherCode = doc["current"]["weather_code"] | -1;
  next.windSpeedKmh = doc["current"]["wind_speed_10m"] | NAN;
  next.windDirectionDeg = doc["current"]["wind_direction_10m"] | NAN;
  strlcpy(next.conditionText, conditionForCode(next.weatherCode), sizeof(next.conditionText));
  // Sunrise and sunset are returned as arrays of ISO-8601 strings.
  // Read the array elements explicitly: using the ArduinoJson default-value
  // operator with nullptr can resolve to the wrong overload on some versions.
  const char* sunriseIso = doc["daily"]["sunrise"][0].as<const char*>();
  const char* sunsetIso  = doc["daily"]["sunset"][0].as<const char*>();
  copyClockText(sunriseIso, next.sunriseText);
  copyClockText(sunsetIso, next.sunsetText);

  Serial.printf("Sun data raw       : %s / %s\n",
                sunriseIso ? sunriseIso : "(missing)",
                sunsetIso ? sunsetIso : "(missing)");
  next.valid = !isnan(next.temperatureC) && !isnan(next.humidityPct) && next.weatherCode >= 0;

  if (!next.valid) {
    Serial.println("Weather error      : response contained incomplete current data");
    return false;
  }

  reading_ = next;
  lastSuccessMs_ = millis();
  Serial.printf("Weather updated    : %.1f C, %.0f%%, %s, wind %.1f km/h @ %.0f deg, sunrise %s, sunset %s\n",
                reading_.temperatureC,
                reading_.humidityPct,
                reading_.conditionText,
                reading_.windSpeedKmh,
                reading_.windDirectionDeg,
                reading_.sunriseText,
                reading_.sunsetText);
  return true;
}
