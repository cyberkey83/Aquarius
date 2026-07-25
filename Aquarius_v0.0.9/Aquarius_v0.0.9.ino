/*
 * Aquarius v0.0.9 — Phase B Ambient Life
 *
 * Focus:
 *   - cleaner information hierarchy
 *   - restrained colour palette
 *   - reusable DisplayManager
 *   - rotating single-line footer
 *   - weather/time responsive aquatic background
 *   - animated water surface, bubbles, fish and plants
 *   - clean dashboard remains the primary focus
 */
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AquariusConfig.h"
#include "AquariusTheme.h"
#include "DisplayManager.h"
#include "IndoorSensor.h"
#include "PressureTrend.h"
#include "TimeService.h"
#include "WeatherService.h"

namespace {

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;
constexpr unsigned long DISPLAY_INTERVAL_MS = 100UL;

TFT_eSPI tft;
TFT_eSprite canvas(&tft);

IndoorSensor indoorSensor;
TimeService timeService;
WeatherService weatherService;
PressureTrend pressureTrend;
DisplayManager displayManager(canvas);

unsigned long lastSensorUpdateMs = 0;
unsigned long lastClockUpdateMs = 0;
unsigned long lastDisplayUpdateMs = 0;

void presentCanvas() {
  canvas.pushSprite(0, 0);
}

void initialiseDisplay() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  canvas.setColorDepth(8);
  if (canvas.createSprite(
          AquariusTheme::SCREEN_WIDTH,
          AquariusTheme::SCREEN_HEIGHT) == nullptr) {
    Serial.println("ERROR: Unable to allocate display sprite");
    while (true) delay(1000);
  }

  canvas.fillSprite(AquariusTheme::BACKGROUND);
}

void renderDashboard(unsigned long nowMs) {
  displayManager.drawDashboard(
      timeService.reading(),
      indoorSensor.reading(),
      weatherService.reading(),
      timeService.wifiConnected(),
      pressureTrend.label(),
      pressureTrend.symbol(),
      nowMs);
  presentCanvas();
}

void printStatus() {
  const IndoorReading& indoor = indoorSensor.reading();
  const OutdoorReading& outdoor = weatherService.reading();

  Serial.println("----------------------------------------");
  Serial.printf(
      "Wi-Fi             : %s\n",
      timeService.wifiConnected() ? "connected" : "disconnected");
  Serial.printf("Indoor sensor     : %s\n", indoorSensor.barometricName());

  if (indoor.temperatureValid) {
    Serial.printf("Indoor temperature: %.2f C\n", indoor.temperatureC);
  }
  if (indoor.humidityValid) {
    Serial.printf("Indoor humidity   : %.2f %%RH\n", indoor.humidityPct);
  } else {
    Serial.println("Indoor humidity   : unavailable");
  }
  if (indoor.pressureValid) {
    Serial.printf("Indoor pressure   : %.2f hPa\n", indoor.pressureHpa);
    Serial.printf(
        "Pressure trend    : %s (%+.2f hPa, %u samples)\n",
        pressureTrend.label(),
        pressureTrend.changeHpa(),
        pressureTrend.sampleCount());
  }

  if (outdoor.valid) {
    Serial.printf("Outdoor temp      : %.2f C\n", outdoor.temperatureC);
    Serial.printf("Outdoor humidity  : %.0f %%RH\n", outdoor.humidityPct);
    Serial.printf("Outdoor condition : %s\n", outdoor.conditionText);
    Serial.printf(
        "Sunrise / sunset  : %s / %s\n",
        outdoor.sunriseText,
        outdoor.sunsetText);
  } else {
    Serial.println("Outdoor weather   : unavailable");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AQUARIUS v0.0.9");
  Serial.println(" Phase B Ambient Life");
  Serial.println("========================================");

  initialiseDisplay();
  displayManager.drawBootScreen();
  presentCanvas();

  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);
  displayManager.drawBootLine(
      String("Sensor: ") + indoorSensor.barometricName(),
      152,
      indoorSensor.hasAnySensor()
          ? AquariusTheme::OUTDOOR
          : AquariusTheme::ERROR_COLOUR);
  presentCanvas();
  delay(400);

  displayManager.drawBootLine(
      "Connecting Wi-Fi...",
      178,
      AquariusTheme::SECONDARY);
  presentCanvas();
  timeService.begin();

  displayManager.drawBootLine(
      timeService.wifiConnected() ? "Wi-Fi connected" : "Wi-Fi unavailable",
      178,
      timeService.wifiConnected()
          ? AquariusTheme::OUTDOOR
          : AquariusTheme::ERROR_COLOUR);
  presentCanvas();

  const unsigned long syncStart = millis();
  while (!timeService.timeValid() && millis() - syncStart < 8000UL) {
    timeService.update();
    delay(250);
  }

  displayManager.drawBootLine(
      "Fetching outdoor weather...",
      204,
      AquariusTheme::SECONDARY);
  presentCanvas();
  weatherService.begin();

  if (indoorSensor.reading().pressureValid) {
    pressureTrend.begin(indoorSensor.reading().pressureHpa);
  }

  const unsigned long now = millis();
  renderDashboard(now);
  printStatus();

  lastSensorUpdateMs = now;
  lastClockUpdateMs = now;
  lastDisplayUpdateMs = now;
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

  if (now - lastDisplayUpdateMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayUpdateMs = now;
    renderDashboard(now);
  }

  delay(1);
}
