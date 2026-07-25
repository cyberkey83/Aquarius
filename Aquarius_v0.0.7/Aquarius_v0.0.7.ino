/*
 * Aquarius v0.0.7 — animated aquarium background
 *
 * Based directly on v0.0.6.
 *
 * Adds:
 *   - subtle animated fish behind the existing dashboard
 *   - gently rising bubbles
 *   - flicker-free full-screen sprite rendering
 *
 * Preserves:
 *   - v0.0.6 dashboard layout
 *   - indoor/outdoor readings
 *   - pressure trend arrows and labels
 *   - sunrise/sunset information
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "AquariusConfig.h"
#include "IndoorSensor.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "PressureTrend.h"
#include "Aquarium.h"

namespace {

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;

constexpr uint16_t SCREEN_WIDTH = 320;
constexpr uint16_t SCREEN_HEIGHT = 240;
constexpr unsigned long AQUARIUM_FRAME_INTERVAL_MS = 100UL;  // 10 FPS

TFT_eSPI tft;
TFT_eSprite canvas(&tft);

IndoorSensor indoorSensor;
TimeService timeService;
WeatherService weatherService;
PressureTrend pressureTrend;
Aquarium aquarium;

unsigned long lastSensorUpdateMs = 0;
unsigned long lastClockUpdateMs = 0;
unsigned long lastAquariumFrameMs = 0;

void presentCanvas() {
  canvas.pushSprite(0, 0);
}

void initialiseDisplay() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Eight-bit colour uses much less RAM than a 16-bit full-screen sprite
  // while retaining more than enough colour for the terminal-style UI.
  canvas.setColorDepth(8);
  if (canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT) == nullptr) {
    Serial.println("ERROR: Unable to allocate display sprite");
    while (true) {
      delay(1000);
    }
  }

  canvas.fillSprite(TFT_BLACK);
}

void drawBootLine(const String& text, int y, uint16_t colour) {
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(colour, TFT_BLACK);
  canvas.fillRect(10, y - 10, 300, 22, TFT_BLACK);
  canvas.drawString(text, 160, y, 2);
  presentCanvas();
}

void drawBootScreen() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString("AQUARIUS", 160, 48, 4);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawString("Indoor + Outdoor Weather", 160, 83, 2);

  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString("v0.0.7", 160, 108, 2);

  drawBootLine("Initialising sensors...", 148, TFT_LIGHTGREY);
}

void drawMainFrame() {
  canvas.drawFastHLine(10, 69, 300, TFT_DARKGREY);
  canvas.drawFastVLine(159, 78, 112, TFT_DARKGREY);
  canvas.drawFastHLine(10, 197, 300, TFT_DARKGREY);

  canvas.setTextDatum(MC_DATUM);

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString("INDOOR", 80, 88, 2);

  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.drawString("OUTDOOR", 239, 88, 2);

  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString("TEMP", 12, 108, 1);
  canvas.drawString("HUM", 12, 145, 1);
  canvas.drawString("TEMP", 171, 108, 1);
  canvas.drawString("HUM", 171, 145, 1);
}

void drawHeader() {
  const ClockReading& clock = timeService.reading();

  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString("AQUARIUS v0.0.7", 8, 6, 2);

  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(
      timeService.wifiConnected() ? TFT_GREEN : TFT_RED,
      TFT_BLACK);
  canvas.drawString(
      timeService.wifiConnected() ? "WIFI OK" : "WIFI --",
      312,
      6,
      2);

  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString(clock.timeText, 160, 34, 4);

  canvas.setTextColor(
      clock.valid ? TFT_WHITE : TFT_YELLOW,
      TFT_BLACK);
  canvas.drawString(clock.dateText, 160, 59, 2);
}

void drawIndoor() {
  const IndoorReading& r = indoorSensor.reading();

  canvas.setTextDatum(TR_DATUM);

  canvas.setTextColor(
      r.temperatureValid ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
      r.temperatureValid
          ? String(r.temperatureC, 1) + " C"
          : "--.- C",
      149,
      117,
      4);

  canvas.setTextColor(
      r.humidityValid ? TFT_CYAN : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
      r.humidityValid
          ? String(r.humidityPct, 0) + " %"
          : "-- %",
      149,
      153,
      4);
}

void drawOutdoor() {
  const OutdoorReading& r = weatherService.reading();

  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(
      r.valid ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
      r.valid
          ? String(r.temperatureC, 1) + " C"
          : "--.- C",
      308,
      117,
      4);

  canvas.setTextColor(
      r.valid ? TFT_CYAN : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
      r.valid
          ? String(r.humidityPct, 0) + " %"
          : "-- %",
      308,
      153,
      4);

  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(
      r.valid ? TFT_WHITE : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
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

  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString("PRESS", 8, 202, 1);

  canvas.setTextColor(
      indoor.pressureValid ? TFT_GREEN : TFT_DARKGREY,
      TFT_BLACK);
  canvas.drawString(
      indoor.pressureValid
          ? String(indoor.pressureHpa, 1) + " hPa " +
                pressureTrend.symbol()
          : "---- hPa",
      47,
      199,
      2);

  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.drawString(pressureTrend.label(), 312, 202, 1);

  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString("SUN", 8, 219, 1);

  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString(
      String(outdoor.sunriseText) + " / " + outdoor.sunsetText,
      34,
      216,
      2);

  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString(
      outdoor.valid && indoor.temperatureValid
          ? comparisonText(indoor.temperatureC, outdoor.temperatureC)
          : AQUARIUS_LOCATION_NAME,
      312,
      219,
      1);
}

void renderScene(unsigned long nowMs) {
  canvas.fillSprite(TFT_BLACK);

  // The aquarium is deliberately drawn first. The unchanged v0.0.6
  // dashboard is then drawn over it, so data always remains readable.
  aquarium.update(nowMs);
  aquarium.draw(canvas);

  drawMainFrame();
  drawHeader();
  drawIndoor();
  drawOutdoor();
  drawFooter();

  presentCanvas();
}

void printStatus() {
  const IndoorReading& indoor = indoorSensor.reading();
  const OutdoorReading& outdoor = weatherService.reading();

  Serial.println("----------------------------------------");
  Serial.printf(
      "Wi-Fi             : %s\n",
      timeService.wifiConnected() ? "connected" : "disconnected");
  Serial.printf(
      "Indoor sensor     : %s\n",
      indoorSensor.barometricName());

  if (indoor.temperatureValid) {
    Serial.printf(
        "Indoor temperature: %.2f C [%s]\n",
        indoor.temperatureC,
        indoor.temperatureSource);
  }

  if (indoor.humidityValid) {
    Serial.printf(
        "Indoor humidity   : %.2f %%RH [%s]\n",
        indoor.humidityPct,
        indoor.humiditySource);
  } else {
    Serial.println("Indoor humidity   : unavailable");
  }

  if (indoor.pressureValid) {
    Serial.printf(
        "Indoor pressure   : %.2f hPa [%s]\n",
        indoor.pressureHpa,
        indoor.pressureSource);
  }

  if (outdoor.valid) {
    Serial.printf(
        "Outdoor temp      : %.2f C\n",
        outdoor.temperatureC);
    Serial.printf(
        "Outdoor humidity  : %.0f %%RH\n",
        outdoor.humidityPct);
    Serial.printf(
        "Outdoor condition : %s (WMO %d)\n",
        outdoor.conditionText,
        outdoor.weatherCode);
    Serial.printf(
        "Outdoor wind      : %.1f km/h\n",
        outdoor.windSpeedKmh);
    Serial.printf(
        "Sunrise / sunset  : %s / %s\n",
        outdoor.sunriseText,
        outdoor.sunsetText);
  } else {
    Serial.println("Outdoor weather   : unavailable");
  }

  if (indoor.pressureValid) {
    Serial.printf(
        "Pressure trend    : %s (%+.2f hPa, %u samples)\n",
        pressureTrend.label(),
        pressureTrend.changeHpa(),
        pressureTrend.sampleCount());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AQUARIUS v0.0.7");
  Serial.println(" Animated aquarium background build");
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

  aquarium.begin(SCREEN_WIDTH, SCREEN_HEIGHT);

  const unsigned long now = millis();
  renderScene(now);
  printStatus();

  lastSensorUpdateMs = now;
  lastClockUpdateMs = now;
  lastAquariumFrameMs = now;
}

void loop() {
  const unsigned long now = millis();

  if (now - lastClockUpdateMs >= AQUARIUS_CLOCK_INTERVAL_MS) {
    lastClockUpdateMs = now;
    timeService.update();
    weatherService.update();
  }

  if (now - lastSensorUpdateMs >= AQUARIUS_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = now;

    indoorSensor.update();
    if (indoorSensor.reading().pressureValid) {
      pressureTrend.update(indoorSensor.reading().pressureHpa);
    }

    printStatus();
  }

  if (now - lastAquariumFrameMs >= AQUARIUM_FRAME_INTERVAL_MS) {
    lastAquariumFrameMs = now;
    renderScene(now);
  }

  delay(1);
}
