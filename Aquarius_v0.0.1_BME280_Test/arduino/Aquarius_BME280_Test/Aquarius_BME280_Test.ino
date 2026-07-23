/*
 * Aquarius v0.0.1 — Environmental sensor hardware test
 *
 * Target:
 *   ESP32-2432S028R / CYD V3
 *
 * Wiring:
 *   CYD 3.3V  -> sensor VCC
 *   CYD GND   -> sensor GND
 *   CYD IO22  -> sensor SDA
 *   CYD IO27  -> sensor SCL
 *
 * The sketch:
 *   1. Scans the I2C bus.
 *   2. Reads the Bosch chip ID.
 *   3. Tries both common addresses: 0x76 and 0x77.
 *   4. Supports genuine BME280 and BMP280 modules.
 *   5. Prints readings every two seconds.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

namespace {
constexpr uint8_t SDA_PIN = 22;
constexpr uint8_t SCL_PIN = 27;

constexpr uint8_t ADDRESS_76 = 0x76;
constexpr uint8_t ADDRESS_77 = 0x77;

constexpr uint8_t CHIP_ID_REGISTER = 0xD0;
constexpr uint8_t BME280_CHIP_ID = 0x60;
constexpr uint8_t BMP280_CHIP_ID_1 = 0x56;
constexpr uint8_t BMP280_CHIP_ID_2 = 0x57;
constexpr uint8_t BMP280_CHIP_ID_3 = 0x58;

constexpr unsigned long READ_INTERVAL_MS = 2000;

Adafruit_BME280 bme;
Adafruit_BMP280 bmp;

enum class SensorType {
  None,
  BME280,
  BMP280
};

SensorType sensorType = SensorType::None;
uint8_t sensorAddress = 0;
unsigned long lastReadMs = 0;

bool addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t readRegister(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return 0xFF;
  }

  const uint8_t received = Wire.requestFrom(
      static_cast<int>(address),
      1,
      static_cast<int>(true));

  if (received != 1 || !Wire.available()) {
    return 0xFF;
  }

  return Wire.read();
}

void scanI2cBus() {
  Serial.println();
  Serial.println(F("Scanning I2C bus..."));

  uint8_t count = 0;

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  Device found at 0x%02X\n", address);
      ++count;
    }
  }

  if (count == 0) {
    Serial.println(F("  No I2C devices found."));
  }
}

bool initialiseAtAddress(uint8_t address) {
  if (!addressResponds(address)) {
    return false;
  }

  const uint8_t chipId = readRegister(address, CHIP_ID_REGISTER);

  Serial.printf(
      "Device at 0x%02X reports chip ID 0x%02X\n",
      address,
      chipId);

  if (chipId == BME280_CHIP_ID) {
    if (!bme.begin(address, &Wire)) {
      Serial.println(F("BME280 chip recognised, but library initialisation failed."));
      return false;
    }

    sensorType = SensorType::BME280;
    sensorAddress = address;
    return true;
  }

  if (chipId == BMP280_CHIP_ID_1 ||
      chipId == BMP280_CHIP_ID_2 ||
      chipId == BMP280_CHIP_ID_3) {
    if (!bmp.begin(address)) {
      Serial.println(F("BMP280 chip recognised, but library initialisation failed."));
      return false;
    }

    sensorType = SensorType::BMP280;
    sensorAddress = address;
    return true;
  }

  Serial.println(F("The device is not recognised as a BME280 or BMP280."));
  return false;
}

bool initialiseSensor() {
  if (initialiseAtAddress(ADDRESS_76)) {
    return true;
  }

  if (initialiseAtAddress(ADDRESS_77)) {
    return true;
  }

  sensorType = SensorType::None;
  sensorAddress = 0;
  return false;
}

void printHeader() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" AQUARIUS v0.0.1"));
  Serial.println(F(" CYD environmental sensor hardware test"));
  Serial.println(F("========================================"));
  Serial.printf("I2C SDA: GPIO %u\n", SDA_PIN);
  Serial.printf("I2C SCL: GPIO %u\n", SCL_PIN);
}

void printSensorDetected() {
  Serial.println();
  Serial.println(F("Sensor initialised successfully."));

  if (sensorType == SensorType::BME280) {
    Serial.println(F("Type: BME280"));
    Serial.println(F("Measurements: temperature, humidity, pressure"));
  } else if (sensorType == SensorType::BMP280) {
    Serial.println(F("Type: BMP280"));
    Serial.println(F("Measurements: temperature and pressure only"));
    Serial.println(F("Humidity is not supported by this chip."));
  }

  Serial.printf("I2C address: 0x%02X\n", sensorAddress);
  Serial.println();
}

void printTroubleshooting() {
  Serial.println();
  Serial.println(F("No compatible sensor was initialised."));
  Serial.println(F("Check the following:"));
  Serial.println(F("  CYD 3.3V -> VCC"));
  Serial.println(F("  CYD GND  -> GND"));
  Serial.println(F("  CYD IO22 -> SDA"));
  Serial.println(F("  CYD IO27 -> SCL"));
  Serial.println(F("If the module still is not detected:"));
  Serial.println(F("  Tie CSB to 3.3V to force I2C mode."));
  Serial.println(F("  Tie SDO to GND for address 0x76,"));
  Serial.println(F("  or tie SDO to 3.3V for address 0x77."));
}

void printReadings() {
  Serial.println(F("----------------------------------------"));

  if (sensorType == SensorType::BME280) {
    const float temperatureC = bme.readTemperature();
    const float humidityPct = bme.readHumidity();
    const float pressureHpa = bme.readPressure() / 100.0F;

    if (isnan(temperatureC) || isnan(humidityPct) || isnan(pressureHpa)) {
      Serial.println(F("Sensor returned an invalid reading."));
      return;
    }

    Serial.printf("Temperature : %.2f C\n", temperatureC);
    Serial.printf("Humidity    : %.2f %%RH\n", humidityPct);
    Serial.printf("Pressure    : %.2f hPa\n", pressureHpa);
    return;
  }

  if (sensorType == SensorType::BMP280) {
    const float temperatureC = bmp.readTemperature();
    const float pressureHpa = bmp.readPressure() / 100.0F;

    if (isnan(temperatureC) || isnan(pressureHpa)) {
      Serial.println(F("Sensor returned an invalid reading."));
      return;
    }

    Serial.printf("Temperature : %.2f C\n", temperatureC);
    Serial.println(F("Humidity    : unavailable (BMP280)"));
    Serial.printf("Pressure    : %.2f hPa\n", pressureHpa);
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  printHeader();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanI2cBus();

  if (!initialiseSensor()) {
    printTroubleshooting();
    return;
  }

  printSensorDetected();
  printReadings();
  lastReadMs = millis();
}

void loop() {
  if (sensorType == SensorType::None) {
    delay(1000);
    return;
  }

  const unsigned long now = millis();

  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    printReadings();
  }
}
