#include "IndoorSensor.h"
#include <math.h>

bool IndoorSensor::tryBme(uint8_t address) {
  if (!bme_.begin(address, wire_)) return false;
  const uint8_t id = bme_.sensorID();
  return id == 0x60;
}

bool IndoorSensor::tryBmp(uint8_t address) {
  if (!bmp_.begin(address, 0x58)) return false;
  const uint8_t id = bmp_.sensorID();
  return id == 0x56 || id == 0x57 || id == 0x58;
}

bool IndoorSensor::begin(TwoWire& wire, uint8_t sdaPin, uint8_t sclPin) {
  wire_ = &wire;
  wire_->begin(sdaPin, sclPin);

  bmeFound_ = tryBme(0x76) || tryBme(0x77);
  if (!bmeFound_) bmpFound_ = tryBmp(0x76) || tryBmp(0x77);

#if AQUARIUS_ENABLE_DHT
  dht_.begin();
  delay(1500);
  const float testHumidity = dht_.readHumidity();
  const float testTemperature = dht_.readTemperature();
  dhtFound_ = !isnan(testHumidity) || !isnan(testTemperature);
#endif

  update();
  return hasAnySensor();
}

void IndoorSensor::update() {
  reading_ = IndoorReading{};

  if (bmeFound_) {
    const float t = bme_.readTemperature();
    const float h = bme_.readHumidity();
    const float p = bme_.readPressure() / 100.0F;
    if (!isnan(t)) { reading_.temperatureC = t; reading_.temperatureValid = true; reading_.temperatureSource = "BME280"; }
    if (!isnan(h)) { reading_.humidityPct = h; reading_.humidityValid = true; reading_.humiditySource = "BME280"; }
    if (!isnan(p) && p > 100.0F) { reading_.pressureHpa = p; reading_.pressureValid = true; reading_.pressureSource = "BME280"; }
  } else if (bmpFound_) {
    const float t = bmp_.readTemperature();
    const float p = bmp_.readPressure() / 100.0F;
    if (!isnan(t)) { reading_.temperatureC = t; reading_.temperatureValid = true; reading_.temperatureSource = "BMP280"; }
    if (!isnan(p) && p > 100.0F) { reading_.pressureHpa = p; reading_.pressureValid = true; reading_.pressureSource = "BMP280"; }
  }

#if AQUARIUS_ENABLE_DHT
  if (dhtFound_) {
    const float t = dht_.readTemperature();
    const float h = dht_.readHumidity();
    if (!reading_.temperatureValid && !isnan(t)) { reading_.temperatureC = t; reading_.temperatureValid = true; reading_.temperatureSource = "DHT22"; }
    if (!isnan(h)) { reading_.humidityPct = h; reading_.humidityValid = true; reading_.humiditySource = "DHT22"; }
  }
#endif

  if (reading_.temperatureValid) reading_.temperatureC += temperatureOffsetC_;
  if (reading_.humidityValid) reading_.humidityPct = constrain(reading_.humidityPct + humidityOffsetPct_, 0.0f, 100.0f);
}

const char* IndoorSensor::barometricName() const {
  if (bmeFound_) return "BME280";
  if (bmpFound_) return "BMP280";
#if AQUARIUS_ENABLE_DHT
  if (dhtFound_) return "DHT22";
#endif
  return "NONE";
}
