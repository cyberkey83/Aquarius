# Aquarius v0.0.3 — Sensor abstraction

This release adds a generic indoor sensor layer so the rest of Aquarius can request temperature, humidity and pressure without caring which physical sensor supplied them.

Supported models:
- BME280: temperature + humidity + pressure
- BMP280: temperature + pressure
- DHT22: temperature + humidity
- BMP280 + DHT22: DHT22 temperature/humidity + BMP280 pressure

DHT22 support is implemented but disabled by default in `AquariusConfig.h`.

Why? The CYD exposes GPIO22, GPIO27 and GPIO35. GPIO22/27 are already used for I2C, while GPIO35 is input-only and cannot perform the DHT22 host signalling. A future hardware note will document a clean alternate GPIO method.

For the current BMP280 setup, install no new library and make no wiring changes.

Keep the known-good ILI9341 `TFT_eSPI/User_Setup.h` already installed.

Open:
`arduino/Aquarius_v0.0.3/Aquarius_v0.0.3.ino`

Settings:
- Board: ESP32 Dev Module
- Upload speed: 460800
- Serial Monitor: 115200

Expected Serial output includes the source of each measurement, e.g.:

Barometric sensor : BMP280
I2C address       : 0x76
DHT support       : disabled
Temperature       : 27.80 C [BMP280]
Humidity          : unavailable
Pressure          : 1003.00 hPa [BMP280]
