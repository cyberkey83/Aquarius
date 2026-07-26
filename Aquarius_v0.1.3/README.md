# Aquarius v0.1.3 — Weather Ecology

Aquarius is an open-source desktop companion for the ESP32-2432S028R "Cheap Yellow Display". It combines indoor sensor data, Open-Meteo outdoor weather and a living aquarium-style ambient display.

## v0.1.3 focus

The dashboard layout is intentionally **locked** in this release. Development is concentrated on the Aquarium Engine.

Weather Ecology turns live outdoor conditions into gradual changes in the aquarium rather than simple on/off effects:

- clear weather increases activity and bubble production
- cloud progressively mutes the aquarium
- rain roughens the surface, adds rainfall/splashes and pushes fish deeper
- storms build stronger surface movement, deeper fish behaviour and brief lightning
- snow creates slow drifting particles and calmer/deeper fish
- fog adds subtle moving haze and reduces activity
- wind affects surface movement and plant sway
- sunrise and sunset use the fetched Open-Meteo times to create gradual lighting transitions
- night lowers fish activity and adds a very restrained bioluminescent ambience
- weather-state changes cross-fade instead of snapping immediately

The v0.1.2 fish personalities, touch feeding, squid, octopus, rare visitors and submarine remain intact.

## Touch controls

**Tap the aquarium:** feed the fish at the tapped position.

A future release will reserve a long press for the settings screen.

## Required Arduino libraries

- TFT_eSPI
- ArduinoJson
- Adafruit BME280 Library
- Adafruit BMP280 Library
- Adafruit Unified Sensor
- XPT2046_Touchscreen by Paul Stoffregen

## Hardware baseline

- ESP32-2432S028R V3 / ESP32-D0WD-V3
- ILI9341 320×240 display
- XPT2046 touch controller
- I2C SDA 22 / SCL 27
- BME280 recommended; BMP280 remains supported

## Build

1. Copy `Secrets.example.h` to `Secrets.h` if required and enter Wi-Fi credentials.
2. Confirm latitude/longitude and hardware settings in `AquariusConfig.h`.
3. Open `Aquarius_v0.1.3.ino` in Arduino IDE.
4. Use the same ESP32 board, partition and upload settings that worked with v0.1.2.
5. Compile and upload.

See `AQUARIUS_DESIGN_DOCUMENT.md` for the project direction and Aquarium Engine roadmap.
