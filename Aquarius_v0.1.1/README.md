# Aquarius v0.0.9 — Phase B: Ambient Life

This is the complete Phase B build based directly on the final working v0.0.8 sunrise/sunset build.

## New in v0.0.9

- Animated water surface beneath the header
- Seven gently rising bubble streams
- Two occasional pixel fish travelling in opposite directions
- Swaying aquatic plants in both lower corners
- Background mood responds to time of day and Open-Meteo weather code
- Dashboard refresh increased to 10 FPS
- All working v0.0.8 functions retained, including seconds, outdoor weather, sunrise and sunset

## Setup

1. Extract the ZIP.
2. Open `Secrets.h` and enter your Wi-Fi details.
3. Open `Aquarius_v0.0.9.ino` in Arduino IDE.
4. Compile and upload using the same CYD settings used for v0.0.8.

The animations are intentionally visible but remain around the dashboard rather than replacing it.


## v0.1.1 interaction update

- Tap anywhere on the touchscreen to feed the fish.
- Food falls near the horizontal position of the tap and the fish turn towards it.
- Requires the **XPT2046_Touchscreen** Arduino library by Paul Stoffregen.
- Default touch pins and calibration values are in `AquariusConfig.h` for the standard ESP32-2432S028R CYD.
- Squid and octopus are now included in the timed rare-visitor pool. The squid darts through mid-water; the octopus crawls slowly along the seabed.
