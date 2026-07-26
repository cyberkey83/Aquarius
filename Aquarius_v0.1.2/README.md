# Aquarius v0.1.2 — Behaviour Pass

Aquarius is an open-source desktop companion for the ESP32-2432S028R (CYD), combining an environmental dashboard with a living aquarium.

## v0.1.2 focus

This release deliberately leaves the successful v0.1.1 dashboard layout alone and concentrates on the Aquarium Engine.

The four normal fish now have individual personalities and independent movement decisions rather than simply travelling back and forth at fixed depths. Feeding has three behavioural phases, and squid/octopus visitors now move in ways that better match their species.

## Interaction

- **Tap the touchscreen:** feed the fish at the tapped location.
- Food falls near the tap.
- Fish accelerate towards it, enter a short feeding frenzy, then gradually recover.

## Aquarium behaviour

Common fish personalities:

- **Calm** — slower, smaller depth changes and fewer turns.
- **Explorer** — wider vertical range and more frequent course changes.
- **Social** — tends to favour the average depth of the school.
- **Darting** — faster movement with occasional short bursts.

Rare visitors currently include koi, jellyfish, angler fish, shark, squid and octopus, plus submarine and nocturnal events.

## Required Arduino libraries

- TFT_eSPI
- ArduinoJson
- Adafruit BME280 Library / Adafruit BMP280 Library
- DHT sensor library if DHT support is enabled
- XPT2046_Touchscreen by Paul Stoffregen

Use the same ESP32 board, partition and TFT_eSPI configuration that worked for Aquarius v0.1.1.

## Setup

1. Extract the project folder.
2. Open `Secrets.h` and enter your Wi-Fi details.
3. Open `Aquarius_v0.1.2.ino` in Arduino IDE.
4. Compile and upload.

Touch calibration constants are in `AquariusConfig.h`.
