/*
 * Aquarius v0.0.4 — Wi-Fi + NTP clock
 *
 * Adds:
 *   - Wi-Fi connection/reconnection
 *   - SNTP internet time
 *   - UK GMT/BST timezone handling
 *   - live date/time display
 *
 * Existing:
 *   - BME280/BMP280 sensor abstraction
 *   - optional DHT22 abstraction
 *   - CYD ILI9341 display
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AquariusConfig.h"
#include "IndoorSensor.h"
#include "TimeService.h"

namespace {

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;

TFT_eSPI tft;
IndoorSensor indoorSensor;
TimeService timeService;

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
  tft.fillRect(20, y - 10, 280, 22, TFT_BLACK);
  tft.drawString(text, 160, y, 2);
}

void drawBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 160, 48, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("ASCII Aquarium Desk Companion", 160, 83, 2);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.4", 160, 108, 2);

  drawBootLine("Initialising sensors...", 148, TFT_LIGHTGREY);
}

void drawMainFrame() {
  tft.fillScreen(TFT_BLACK);

  // Header divider
  tft.drawFastHLine(10, 78, 300, TFT_DARKGREY);

  // Sensor section labels
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("TEMP", 16, 102, 2);
  tft.drawString("HUM", 16, 137, 2);
  tft.drawString("PRESS", 16, 172, 2);

  // Footer divider
  tft.drawFastHLine(10, 205, 300, TFT_DARKGREY);
}

void drawClock() {
  const ClockReading& c = timeService.reading();

  // Clear complete clock/date/header area.
  tft.fillRect(0, 0, 320, 76, TFT_BLACK);

  // Version left
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("AQUARIUS v0.0.4", 8, 7, 2);

  // Wi-Fi right
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(
      timeService.wifiConnected() ? TFT_GREEN : TFT_RED,
      TFT_BLACK);
  tft.drawString(timeService.wifiStatusText(), 312, 7, 2);

  // Time
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(c.timeText, 160, 38, 4);

  // Date/status
  tft.setTextColor(
      c.valid ? TFT_WHITE : TFT_YELLOW,
      TFT_BLACK);
  tft.drawString(c.dateText, 160, 65, 2);
}

void drawSensorValues() {
  const IndoorReading& r = indoorSensor.reading();

  // Numeric column
  tft.setTextDatum(TR_DATUM);

  tft.fillRect(105, 92, 200, 31, TFT_BLACK);
  tft.setTextColor(
      r.temperatureValid ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.temperatureValid
          ? String(r.temperatureC, 1) + " C"
          : "--.- C",
      300, 96, 4);

  tft.fillRect(105, 127, 200, 31, TFT_BLACK);
  tft.setTextColor(
      r.humidityValid ? TFT_CYAN : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.humidityValid
          ? String(r.humidityPct, 1) + " %"
          : "-- %",
      300, 131, 4);

  tft.fillRect(105, 162, 200, 31, TFT_BLACK);
  tft.setTextColor(
      r.pressureValid ? TFT_GREEN : TFT_DARKGREY,
      TFT_BLACK);
  tft.drawString(
      r.pressureValid
          ? String(r.pressureHpa, 1) + " hPa"
          : "---- hPa",
      300, 166, 4);

  // Footer
  tft.fillRect(10, 208, 300, 28, TFT_BLACK);

  String footer = indoorSensor.barometricName();

#if AQUARIUS_ENABLE_DHT
  if (indoorSensor.dhtResponding()) {
    if (indoorSensor.barometricType() != BarometricSensorType::None) {
      footer += " + DHT22";
    } else {
      footer = "DHT22";
    }
  }
#endif

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("SENSOR: " + footer, 12, 216, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(
      indoorSensor.hasAnySensor() ? TFT_GREEN : TFT_RED,
      TFT_BLACK);
  tft.drawString(
      indoorSensor.hasAnySensor() ? "OK" : "ERROR",
      308, 216, 2);
}

void printStatus() {
  const ClockReading& c = timeService.reading();
  const IndoorReading& r = indoorSensor.reading();

  Serial.println("----------------------------------------");
  Serial.printf("Wi-Fi             : %s\n",
                timeService.wifiConnected() ? "connected" : "disconnected");
  Serial.printf("Time               : %s\n", c.timeText);
  Serial.printf("Date               : %s\n", c.dateText);
  Serial.printf("Barometric sensor : %s\n", indoorSensor.barometricName());

  if (r.temperatureValid) {
    Serial.printf("Temperature       : %.2f C [%s]\n",
                  r.temperatureC,
                  r.temperatureSource);
  }

  if (r.humidityValid) {
    Serial.printf("Humidity          : %.2f %%RH [%s]\n",
                  r.humidityPct,
                  r.humiditySource);
  } else {
    Serial.println("Humidity          : unavailable");
  }

  if (r.pressureValid) {
    Serial.printf("Pressure          : %.2f hPa [%s]\n",
                  r.pressureHpa,
                  r.pressureSource);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AQUARIUS v0.0.4");
  Serial.println(" Wi-Fi + NTP clock build");
  Serial.println("========================================");

  initialiseDisplay();
  drawBootScreen();

  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);

  drawBootLine(
      String("Sensor: ") + indoorSensor.barometricName(),
      148,
      indoorSensor.hasAnySensor() ? TFT_GREEN : TFT_RED);

  delay(500);

  drawBootLine("Connecting Wi-Fi...", 175, TFT_LIGHTGREY);
  timeService.begin();

  drawBootLine(
      timeService.wifiConnected()
          ? "Wi-Fi connected"
          : "Wi-Fi unavailable",
      175,
      timeService.wifiConnected() ? TFT_GREEN : TFT_RED);

  drawBootLine(
      "Synchronising time...",
      202,
      TFT_LIGHTGREY);

  // Give the first NTP sync a few seconds without blocking forever.
  const unsigned long syncStart = millis();

  while (!timeService.timeValid() &&
         millis() - syncStart < 8000UL) {
    timeService.update();
    delay(250);
  }

  drawMainFrame();
  drawClock();
  drawSensorValues();
  printStatus();

  lastSensorUpdateMs = millis();
  lastClockUpdateMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastClockUpdateMs >= AQUARIUS_CLOCK_INTERVAL_MS) {
    lastClockUpdateMs = now;

    timeService.update();
    drawClock();
  }

  if (now - lastSensorUpdateMs >= AQUARIUS_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = now;

    indoorSensor.update();
    drawSensorValues();
    printStatus();
  }
}
