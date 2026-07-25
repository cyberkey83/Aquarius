/*
 * Aquarius v0.0.6 — Open-Meteo outdoor weather
 *
 * Adds:
 *   - current outdoor temperature
 *   - current outdoor humidity
 *   - WMO weather condition
 *   - indoor/outdoor dashboard
 *
 * Open-Meteo location is configured in AquariusConfig.h.
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AquariusConfig.h"
#include "IndoorSensor.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "PressureTrend.h"

namespace {

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;

TFT_eSPI tft;
IndoorSensor indoorSensor;
TimeService timeService;
WeatherService weatherService;
PressureTrend pressureTrend;

unsigned long lastSensorUpdateMs = 0;
unsigned long lastClockUpdateMs = 0;

void initialiseDisplay() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void drawBootLine(const String& text, int y, uint16_t colour) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(colour, TFT_BLACK);
  tft.fillRect(10, y - 10, 300, 22, TFT_BLACK);
  tft.drawString(text, 160, y, 2);
}

void drawBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 160, 48, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Indoor + Outdoor Weather", 160, 83, 2);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.5", 160, 108, 2);

  drawBootLine("Initialising sensors...", 148, TFT_LIGHTGREY);
}

void drawMainFrame() {
  tft.fillScreen(TFT_BLACK);

  tft.drawFastHLine(10, 69, 300, TFT_DARKGREY);
  tft.drawFastVLine(159, 78, 112, TFT_DARKGREY);
  tft.drawFastHLine(10, 197, 300, TFT_DARKGREY);

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("INDOOR", 80, 88, 2);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("OUTDOOR", 239, 88, 2);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("TEMP", 12, 108, 1);
  tft.drawString("HUM", 12, 145, 1);

  tft.drawString("TEMP", 171, 108, 1);
  tft.drawString("HUM", 171, 145, 1);
}

void drawHeader() {
  const ClockReading& clock = timeService.reading();

  tft.fillRect(0, 0, 320, 67, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("AQUARIUS v0.0.6", 8, 6, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(
      timeService.wifiConnected() ? TFT_GREEN : TFT_RED,
      TFT_BLACK);
  tft.drawString(
      timeService.wifiConnected() ? "WIFI OK" : "WIFI --",
      312,
      6,
      2);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(clock.timeText, 160, 34, 4);

  tft.setTextColor(
      clock.valid ? TFT_WHITE : TFT_YELLOW,
      TFT_BLACK);
  tft.drawString(clock.dateText, 160, 59, 2);
}

void drawIndoor() {
  const IndoorReading& r = indoorSensor.reading();

  tft.fillRect(18, 118, 133, 66, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);

  tft.setTextColor(
      r.temperatureValid ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.temperatureValid
          ? String(r.temperatureC, 1) + " C"
          : "--.- C",
      149,
      117,
      4);

  tft.setTextColor(
      r.humidityValid ? TFT_CYAN : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.humidityValid
          ? String(r.humidityPct, 0) + " %"
          : "-- %",
      149,
      153,
      4);
}

void drawOutdoor() {
  const OutdoorReading& r = weatherService.reading();

  tft.fillRect(177, 118, 133, 68, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);

  tft.setTextColor(
      r.valid ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.valid
          ? String(r.temperatureC, 1) + " C"
          : "--.- C",
      308,
      117,
      4);

  tft.setTextColor(
      r.valid ? TFT_CYAN : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.valid
          ? String(r.humidityPct, 0) + " %"
          : "-- %",
      308,
      153,
      4);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(
      r.valid ? TFT_WHITE : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.valid ? r.conditionText : "WAITING...",
      239,
      183,
      1);
}


const char* comparisonText(float indoor, float outdoor) {
  if (isnan(indoor) || isnan(outdoor)) return "";
  const float difference = indoor - outdoor;
  if (difference > 0.5F) return "IN WARMER";
  if (difference < -0.5F) return "OUT WARMER";
  return "TEMP SIMILAR";
}

void drawFooter() {
  const IndoorReading& indoor = indoorSensor.reading();
  const OutdoorReading& outdoor = weatherService.reading();

  tft.fillRect(0, 198, 320, 42, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("PRESS", 8, 202, 1);

  tft.setTextColor(
      indoor.pressureValid ? TFT_GREEN : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      indoor.pressureValid
          ? String(indoor.pressureHpa, 1) + " hPa " +
                pressureTrend.symbol()
          : "---- hPa",
      47, 199, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(pressureTrend.label(), 312, 202, 1);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("SUN", 8, 219, 1);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(
      String(outdoor.sunriseText) + " / " + outdoor.sunsetText,
      34, 216, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(
      outdoor.valid && indoor.temperatureValid
          ? comparisonText(indoor.temperatureC, outdoor.temperatureC)
          : AQUARIUS_LOCATION_NAME,
      312, 219, 1);
}

void printStatus() {
  const IndoorReading& indoor = indoorSensor.reading();
  const OutdoorReading& outdoor = weatherService.reading();

  Serial.println("----------------------------------------");
  Serial.printf("Wi-Fi             : %s\n",
                timeService.wifiConnected() ? "connected" : "disconnected");
  Serial.printf("Indoor sensor     : %s\n",
                indoorSensor.barometricName());

  if (indoor.temperatureValid) {
    Serial.printf("Indoor temperature: %.2f C [%s]\n",
                  indoor.temperatureC,
                  indoor.temperatureSource);
  }

  if (indoor.humidityValid) {
    Serial.printf("Indoor humidity   : %.2f %%RH [%s]\n",
                  indoor.humidityPct,
                  indoor.humiditySource);
  } else {
    Serial.println("Indoor humidity   : unavailable");
  }

  if (indoor.pressureValid) {
    Serial.printf("Indoor pressure   : %.2f hPa [%s]\n",
                  indoor.pressureHpa,
                  indoor.pressureSource);
  }

  if (outdoor.valid) {
    Serial.printf("Outdoor temp      : %.2f C\n", outdoor.temperatureC);
    Serial.printf("Outdoor humidity  : %.0f %%RH\n", outdoor.humidityPct);
    Serial.printf("Outdoor condition : %s (WMO %d)\n",
                  outdoor.conditionText,
                  outdoor.weatherCode);
    Serial.printf("Outdoor wind      : %.1f km/h\n", outdoor.windSpeedKmh);
    Serial.printf("Sunrise / sunset  : %s / %s\n", outdoor.sunriseText, outdoor.sunsetText);
  } else {
    Serial.println("Outdoor weather   : unavailable");
  }

  if (indoor.pressureValid) {
    Serial.printf("Pressure trend    : %s (%+.2f hPa, %u samples)\n",
                  pressureTrend.label(), pressureTrend.changeHpa(),
                  pressureTrend.sampleCount());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AQUARIUS v0.0.6");
  Serial.println(" Open-Meteo outdoor weather build");
  Serial.println("========================================");

  initialiseDisplay();
  drawBootScreen();

  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);

  drawBootLine(
      String("Sensor: ") + indoorSensor.barometricName(),
      148,
      indoorSensor.hasAnySensor() ? TFT_GREEN : TFT_RED);

  delay(400);
  drawBootLine("Connecting Wi-Fi...", 175, TFT_LIGHTGREY);
  timeService.begin();

  drawBootLine(
      timeService.wifiConnected()
          ? "Wi-Fi connected"
          : "Wi-Fi unavailable",
      175,
      timeService.wifiConnected() ? TFT_GREEN : TFT_RED);

  const unsigned long syncStart = millis();
  while (!timeService.timeValid() &&
         millis() - syncStart < 8000UL) {
    timeService.update();
    delay(250);
  }

  drawBootLine("Fetching outdoor weather...", 202, TFT_LIGHTGREY);
  weatherService.begin();

  if (indoorSensor.reading().pressureValid) {
    pressureTrend.begin(indoorSensor.reading().pressureHpa);
  }

  drawMainFrame();
  drawHeader();
  drawIndoor();
  drawOutdoor();
  drawFooter();
  printStatus();

  lastSensorUpdateMs = millis();
  lastClockUpdateMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastClockUpdateMs >= AQUARIUS_CLOCK_INTERVAL_MS) {
    lastClockUpdateMs = now;
    timeService.update();
    weatherService.update();
    drawHeader();
  }

  if (now - lastSensorUpdateMs >= AQUARIUS_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = now;

    indoorSensor.update();
    if (indoorSensor.reading().pressureValid) {
      pressureTrend.update(indoorSensor.reading().pressureHpa);
    }
    drawIndoor();
    drawOutdoor();
    drawFooter();
    printStatus();
  }
}
