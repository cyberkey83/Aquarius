#include "IndoorSensor.h"

namespace {
constexpr uint8_t CHIP_ID_REGISTER = 0xD0;
}

IndoorSensor::IndoorSensor()
#if AQUARIUS_ENABLE_DHT
    : dht_(AQUARIUS_DHT_PIN, DHT22)
#endif
{
}

bool IndoorSensor::begin(TwoWire& wire, uint8_t sdaPin, uint8_t sclPin) {
  wire_ = &wire;
  wire_->begin(sdaPin, sclPin);
  wire_->setClock(100000);

  const bool baroFound = beginBarometric();

#if AQUARIUS_ENABLE_DHT
  dht_.begin();
  delay(2100);
  readDht();
#endif

  update();
  return baroFound || dhtResponding_;
}

bool IndoorSensor::update() {
  readBarometric();

#if AQUARIUS_ENABLE_DHT
  readDht();
#endif

  combineReadings();

  return reading_.temperatureValid ||
         reading_.humidityValid ||
         reading_.pressureValid;
}

const char* IndoorSensor::barometricName() const {
  switch (barometricType_) {
    case BarometricSensorType::BME280: return "BME280";
    case BarometricSensorType::BMP280: return "BMP280";
    default: return "NONE";
  }
}

bool IndoorSensor::hasAnySensor() const {
  return barometricType_ != BarometricSensorType::None || dhtResponding_;
}

bool IndoorSensor::dhtEnabled() const {
#if AQUARIUS_ENABLE_DHT
  return true;
#else
  return false;
#endif
}

bool IndoorSensor::beginBarometric() {
  return beginBarometricAt(0x76) || beginBarometricAt(0x77);
}

bool IndoorSensor::beginBarometricAt(uint8_t address) {
  if (!addressResponds(address)) {
    return false;
  }

  const uint8_t id = readRegister(address, CHIP_ID_REGISTER);
  Serial.printf(
      "I2C device 0x%02X reports chip ID 0x%02X\n",
      address,
      id);

  if (id == 0x60 && bme_.begin(address, wire_)) {
    barometricType_ = BarometricSensorType::BME280;
    barometricAddress_ = address;
    return true;
  }

  if ((id == 0x56 || id == 0x57 || id == 0x58) &&
      bmp_.begin(address)) {
    barometricType_ = BarometricSensorType::BMP280;
    barometricAddress_ = address;
    return true;
  }

  return false;
}

uint8_t IndoorSensor::readRegister(uint8_t address, uint8_t reg) {
  wire_->beginTransmission(address);
  wire_->write(reg);

  if (wire_->endTransmission(false) != 0) {
    return 0xFF;
  }

  if (wire_->requestFrom((int)address, 1, true) != 1) {
    return 0xFF;
  }

  return wire_->available() ? wire_->read() : 0xFF;
}

bool IndoorSensor::addressResponds(uint8_t address) {
  wire_->beginTransmission(address);
  return wire_->endTransmission() == 0;
}

void IndoorSensor::readBarometric() {
  baroTemperatureC_ = NAN;
  baroHumidityPct_ = NAN;
  baroPressureHpa_ = NAN;

  if (barometricType_ == BarometricSensorType::BME280) {
    baroTemperatureC_ = bme_.readTemperature();
    baroHumidityPct_ = bme_.readHumidity();
    baroPressureHpa_ = bme_.readPressure() / 100.0F;
  } else if (barometricType_ == BarometricSensorType::BMP280) {
    baroTemperatureC_ = bmp_.readTemperature();
    baroPressureHpa_ = bmp_.readPressure() / 100.0F;
  }
}

void IndoorSensor::readDht() {
#if AQUARIUS_ENABLE_DHT
  dhtTemperatureC_ = dht_.readTemperature();
  dhtHumidityPct_ = dht_.readHumidity();

  dhtResponding_ =
      !isnan(dhtTemperatureC_) &&
      !isnan(dhtHumidityPct_);
#else
  dhtTemperatureC_ = NAN;
  dhtHumidityPct_ = NAN;
  dhtResponding_ = false;
#endif
}

void IndoorSensor::combineReadings() {
  reading_ = IndoorReading();

  if (barometricType_ == BarometricSensorType::BME280) {
    if (!isnan(baroTemperatureC_)) {
      reading_.temperatureC = baroTemperatureC_;
      reading_.temperatureValid = true;
      reading_.temperatureSource = "BME280";
    }

    if (!isnan(baroHumidityPct_)) {
      reading_.humidityPct = baroHumidityPct_;
      reading_.humidityValid = true;
      reading_.humiditySource = "BME280";
    }

    if (!isnan(baroPressureHpa_)) {
      reading_.pressureHpa = baroPressureHpa_;
      reading_.pressureValid = true;
      reading_.pressureSource = "BME280";
    }

    return;
  }

  if (dhtResponding_) {
    reading_.temperatureC = dhtTemperatureC_;
    reading_.temperatureValid = true;
    reading_.temperatureSource = "DHT22";

    reading_.humidityPct = dhtHumidityPct_;
    reading_.humidityValid = true;
    reading_.humiditySource = "DHT22";
  } else if (!isnan(baroTemperatureC_)) {
    reading_.temperatureC = baroTemperatureC_;
    reading_.temperatureValid = true;
    reading_.temperatureSource = "BMP280";
  }

  if (!isnan(baroPressureHpa_)) {
    reading_.pressureHpa = baroPressureHpa_;
    reading_.pressureValid = true;
    reading_.pressureSource = "BMP280";
  }
}
