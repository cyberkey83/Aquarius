# Aquarius v0.0.6 — Trends and sunlight

## New features

- Indoor pressure history
- Rising, steady or falling pressure indication
- Learning state until enough samples have been collected
- Today's sunrise and sunset
- Indoor/outdoor temperature comparison
- Existing indoor sensor, Open-Meteo, Wi-Fi and NTP features retained

## Pressure trend behaviour

Aquarius records one pressure sample every five minutes and retains seven
samples. The trend therefore develops over roughly thirty minutes.

- More than +0.5 hPa: rising
- Less than -0.5 hPa: falling
- Between those values: steady
- Fewer than three samples: learning

The history currently resets after a reboot. Persistent history can be added
later using ESP32 Preferences/NVS.

## Sunrise and sunset

The Open-Meteo request now includes today's `sunrise` and `sunset` daily
fields using the location timezone.

## Libraries

The dependencies are unchanged from v0.0.5:

- TFT_eSPI
- ArduinoJson 7
- Adafruit Unified Sensor
- Adafruit BME280 Library
- Adafruit BMP280 Library

## Upload

Copy `Secrets.example.h` to `Secrets.h`, enter your Wi-Fi credentials, then
open `Aquarius_v0.0.6.ino`.

- Board: ESP32 Dev Module
- Upload speed: 460800
- Serial Monitor: 115200

Suggested commit:

`Aquarius v0.0.6 - Add pressure trends and sunrise sunset`
