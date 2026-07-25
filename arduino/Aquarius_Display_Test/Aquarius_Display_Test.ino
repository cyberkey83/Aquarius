#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

constexpr uint8_t SDA_PIN = 22;
constexpr uint8_t SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;
constexpr uint8_t CHIP_ID_REGISTER = 0xD0;
constexpr unsigned long READ_INTERVAL_MS = 2000;

TFT_eSPI tft;
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;

enum class SensorType { None, BME280, BMP280 };
SensorType sensorType = SensorType::None;
uint8_t sensorAddress = 0;
unsigned long lastReadMs = 0;
float temperatureC = NAN, humidityPct = NAN, pressureHpa = NAN;

bool responds(uint8_t a) {
  Wire.beginTransmission(a);
  return Wire.endTransmission() == 0;
}

uint8_t readReg(uint8_t a, uint8_t reg) {
  Wire.beginTransmission(a);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  if (Wire.requestFrom((int)a, 1, true) != 1) return 0xFF;
  return Wire.available() ? Wire.read() : 0xFF;
}

bool initAt(uint8_t a) {
  if (!responds(a)) return false;
  uint8_t id = readReg(a, CHIP_ID_REGISTER);
  Serial.printf("Device at 0x%02X chip ID 0x%02X\n", a, id);

  if (id == 0x60 && bme.begin(a, &Wire)) {
    sensorType = SensorType::BME280;
    sensorAddress = a;
    return true;
  }
  if ((id == 0x56 || id == 0x57 || id == 0x58) && bmp.begin(a)) {
    sensorType = SensorType::BMP280;
    sensorAddress = a;
    return true;
  }
  return false;
}

bool initSensor() {
  return initAt(0x76) || initAt(0x77);
}

const char* sensorName() {
  if (sensorType == SensorType::BME280) return "BME280";
  if (sensorType == SensorType::BMP280) return "BMP280";
  return "NONE";
}

void scanI2C() {
  Serial.println("Scanning I2C bus...");
  int count = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Found 0x%02X\n", a);
      count++;
    }
  }
  if (!count) Serial.println("  No devices found");
}

void bootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 160, 72, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("ASCII Aquarium Desk Companion", 160, 112, 2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.2", 160, 140, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Initialising...", 160, 182, 2);
}

void drawFrame() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 10, 8, 4);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("v0.0.2", 310, 14, 2);

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

  String footer = String("SENSOR: ") + sensorName() +
                  "   I2C: 0x" + String(sensorAddress, HEX);
  footer.toUpperCase();

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString(footer, 12, 210, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("OK", 308, 210, 2);
}

void drawNoSensor() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("AQUARIUS", 12, 10, 4);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("SENSOR ERROR", 12, 70, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("No BME280/BMP280 detected", 12, 115, 2);
  tft.drawString("SDA=IO22  SCL=IO27", 12, 150, 2);
}

void readSensor() {
  if (sensorType == SensorType::BME280) {
    temperatureC = bme.readTemperature();
    humidityPct = bme.readHumidity();
    pressureHpa = bme.readPressure() / 100.0F;
  } else if (sensorType == SensorType::BMP280) {
    temperatureC = bmp.readTemperature();
    humidityPct = NAN;
    pressureHpa = bmp.readPressure() / 100.0F;
  }

  Serial.printf("Temperature : %.2f C\n", temperatureC);
  if (isnan(humidityPct)) Serial.println("Humidity    : unavailable");
  else Serial.printf("Humidity    : %.2f %%RH\n", humidityPct);
  Serial.printf("Pressure    : %.2f hPa\n", pressureHpa);
}

void drawValues() {
  tft.setTextDatum(TR_DATUM);

  tft.fillRect(135, 86, 170, 30, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(isnan(temperatureC) ? "--.- C" : String(temperatureC, 1) + " C", 300, 88, 4);

  tft.fillRect(135, 120, 170, 30, TFT_BLACK);
  tft.setTextColor(isnan(humidityPct) ? TFT_DARKGREY : TFT_CYAN, TFT_BLACK);
  tft.drawString(isnan(humidityPct) ? "-- %" : String(humidityPct, 1) + " %", 300, 122, 4);

  tft.fillRect(135, 154, 170, 30, TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(isnan(pressureHpa) ? "---- hPa" : String(pressureHpa, 1) + " hPa", 300, 156, 4);
}

void setup() {
  Serial.begin(115200);
  delay(800);

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  bootScreen();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  scanI2C();

  delay(800);

  if (!initSensor()) {
    Serial.println("No compatible sensor detected");
    drawNoSensor();
    return;
  }

  Serial.printf("Detected %s at 0x%02X\n", sensorName(), sensorAddress);
  readSensor();
  drawFrame();
  drawValues();
  lastReadMs = millis();
}

void loop() {
  if (sensorType == SensorType::None) {
    delay(1000);
    return;
  }

  unsigned long now = millis();
  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    readSensor();
    drawValues();
  }
}
