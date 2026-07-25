# Aquarius v0.0.7

Complete Arduino sketch package for the ESP32-2432S028R Cheap Yellow Display.

## What changed from v0.0.6

The existing dashboard layout and functionality are preserved. Version 0.0.7
adds a restrained aquarium animation behind the data:

- three muted fish
- seven rising bubbles
- approximately 10 FPS
- dashboard always rendered above the aquarium
- pressure trend arrows, labels and readings retained

No seaweed, weather effects, layout redesign or other major additions are
included in this first pass.

## Before compiling

Open `Secrets.h` and enter your Wi-Fi network name and password.

## Required Arduino libraries

Install these through Arduino Library Manager:

- TFT_eSPI
- Adafruit BME280 Library
- Adafruit BMP280 Library
- Adafruit Unified Sensor
- ArduinoJson
- DHT sensor library — only required when `AQUARIUS_ENABLE_DHT` is set to `1`

## TFT_eSPI display configuration

Use the working CYD pin configuration:

```text
MISO 12
MOSI 13
SCLK 14
CS   15
DC    2
RST   4
BL   21
```

The display driver is ILI9341 and the resolution is 320 × 240.

## Upload

1. Extract the ZIP.
2. Open `Aquarius_v0.0.7/Aquarius_v0.0.7.ino`.
3. Edit `Secrets.h`.
4. Select the same ESP32 board and settings used for v0.0.6.
5. Compile and upload.

Suggested settings:

- Serial monitor: 115200 baud
- Upload speed: 460800 baud
