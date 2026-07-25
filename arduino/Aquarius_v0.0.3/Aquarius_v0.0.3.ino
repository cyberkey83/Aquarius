#include <Arduino.h>
#include <TFT_eSPI.h>
#include "AquariusConfig.h"
#include "IndoorSensor.h"

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;

TFT_eSPI tft;
IndoorSensor indoorSensor;
unsigned long lastSensorUpdateMs = 0;

void drawBootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 160, 72, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Sensor abstraction", 160, 112, 2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.3", 160, 140, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Detecting sensors...", 160, 182, 2);
}

void drawFrame() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 10, 8, 4);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.3", 310, 14, 2);

  tft.drawFastHLine(10, 42, 300, TFT_DARKGREY);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("INDOOR CLIMATE", 160, 62, 2);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("TEMP", 22, 92, 2);
  tft.drawString("HUMIDITY", 22, 126, 2);
  tft.drawString("PRESSURE", 22, 160, 2);

  tft.drawFastHLine(10, 198, 300, TFT_DARKGREY);
}

void drawValues() {
  const IndoorReading& r = indoorSensor.reading();
  tft.setTextDatum(TR_DATUM);

  tft.fillRect(135, 85, 170, 31, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(r.temperatureValid ? String(r.temperatureC, 1) + " C" : "--.- C", 300, 88, 4);

  tft.fillRect(135, 119, 170, 31, TFT_BLACK);
  tft.setTextColor(r.humidityValid ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft.drawString(r.humidityValid ? String(r.humidityPct, 1) + " %" : "-- %", 300, 122, 4);

  tft.fillRect(135, 153, 170, 31, TFT_BLACK);
  tft.setTextColor(r.pressureValid ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
  tft.drawString(r.pressureValid ? String(r.pressureHpa, 1) + " hPa" : "---- hPa", 300, 156, 4);

  tft.fillRect(10, 202, 300, 30, TFT_BLACK);
  String footer = indoorSensor.barometricName();
#if AQUARIUS_ENABLE_DHT
  if (indoorSensor.dhtResponding()) {
    if (indoorSensor.barometricType() != BarometricSensorType::None) footer += " + DHT22";
    else footer = "DHT22";
  }
#endif
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("SENSOR: " + footer, 12, 210, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(indoorSensor.hasAnySensor() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(indoorSensor.hasAnySensor() ? "OK" : "ERROR", 308, 210, 2);
}

void printReading() {
  const IndoorReading& r = indoorSensor.reading();
  Serial.println("----------------------------------------");
  Serial.printf("Barometric sensor : %s\n", indoorSensor.barometricName());
  if (indoorSensor.barometricType() != BarometricSensorType::None)
    Serial.printf("I2C address       : 0x%02X\n", indoorSensor.barometricAddress());
  Serial.printf("DHT support       : %s\n", indoorSensor.dhtEnabled() ? "enabled" : "disabled");

  if (r.temperatureValid) Serial.printf("Temperature       : %.2f C [%s]\n", r.temperatureC, r.temperatureSource);
  else Serial.println("Temperature       : unavailable");

  if (r.humidityValid) Serial.printf("Humidity          : %.2f %%RH [%s]\n", r.humidityPct, r.humiditySource);
  else Serial.println("Humidity          : unavailable");

  if (r.pressureValid) Serial.printf("Pressure          : %.2f hPa [%s]\n", r.pressureHpa, r.pressureSource);
  else Serial.println("Pressure          : unavailable");
}

void setup() {
  Serial.begin(115200);
  delay(800);

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  drawBootScreen();

  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);

  delay(600);
  drawFrame();
  drawValues();
  printReading();
  lastSensorUpdateMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSensorUpdateMs >= AQUARIUS_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = now;
    indoorSensor.update();
    drawValues();
    printReading();
  }
}
