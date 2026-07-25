#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include "AquariusConfig.h"

#if AQUARIUS_ENABLE_DHT
#include <DHT.h>
#endif

enum class BarometricSensorType : uint8_t { None, BME280, BMP280 };

struct IndoorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float pressureHpa = NAN;

  bool temperatureValid = false;
  bool humidityValid = false;
  bool pressureValid = false;

  const char* temperatureSource = "NONE";
  const char* humiditySource = "NONE";
  const char* pressureSource = "NONE";
};

class IndoorSensor {
 public:
  IndoorSensor();

  bool begin(TwoWire& wire, uint8_t sdaPin, uint8_t sclPin);
  bool update();

  const IndoorReading& reading() const { return reading_; }

  BarometricSensorType barometricType() const { return barometricType_; }
  const char* barometricName() const;
  uint8_t barometricAddress() const { return barometricAddress_; }

  bool hasAnySensor() const;
  bool dhtEnabled() const;
  bool dhtResponding() const { return dhtResponding_; }

 private:
  bool beginBarometric();
  bool beginBarometricAt(uint8_t address);
  uint8_t readRegister(uint8_t address, uint8_t reg);
  bool addressResponds(uint8_t address);

  void readBarometric();
  void readDht();
  void combineReadings();

  TwoWire* wire_ = nullptr;

  Adafruit_BME280 bme_;
  Adafruit_BMP280 bmp_;

#if AQUARIUS_ENABLE_DHT
  DHT dht_;
#endif

  BarometricSensorType barometricType_ = BarometricSensorType::None;
  uint8_t barometricAddress_ = 0;
  bool dhtResponding_ = false;

  float baroTemperatureC_ = NAN;
  float baroHumidityPct_ = NAN;
  float baroPressureHpa_ = NAN;

  float dhtTemperatureC_ = NAN;
  float dhtHumidityPct_ = NAN;

  IndoorReading reading_;
};
