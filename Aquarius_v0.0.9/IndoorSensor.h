#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include "AquariusConfig.h"

#if AQUARIUS_ENABLE_DHT
#include <DHT.h>
#endif

struct IndoorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float pressureHpa = NAN;
  bool temperatureValid = false;
  bool humidityValid = false;
  bool pressureValid = false;
  const char* temperatureSource = "none";
  const char* humiditySource = "none";
  const char* pressureSource = "none";
};

class IndoorSensor {
 public:
  bool begin(TwoWire& wire, uint8_t sdaPin, uint8_t sclPin);
  void update();
  const IndoorReading& reading() const { return reading_; }
  bool hasAnySensor() const { return bmeFound_ || bmpFound_ || dhtFound_; }
  const char* barometricName() const;

 private:
  TwoWire* wire_ = nullptr;
  Adafruit_BME280 bme_;
  Adafruit_BMP280 bmp_;
#if AQUARIUS_ENABLE_DHT
  DHT dht_{AQUARIUS_DHT_PIN, DHT22};
#endif
  IndoorReading reading_;
  bool bmeFound_ = false;
  bool bmpFound_ = false;
  bool dhtFound_ = false;

  bool tryBme(uint8_t address);
  bool tryBmp(uint8_t address);
};
