# Aquarius v0.1.7 — Aquarium Life

Aquarius is an open-source CYD (ESP32-2432S028R) desktop weather companion with a living Aquarium Engine.

## v0.1.7 additions
- New persistent **Plant length** setting (1–10), independent of plant density.
- More natural resident fish schooling and gentle separation behaviour.
- Improved bubble streams with grouped rising bubbles, drift and micro-bubbles.
- Refined crab routine with longer hiding periods, less mechanical peeking and varied exploration distance.
- Improved squid and octopus sprite animation/details while retaining rare-event behaviour.
- Fixed the boot fish dorsal fin so it no longer appears clipped beneath the body layer.
- About screen now shows **SSID, IP address and MAC address** alongside firmware, Wi-Fi, sensor, location and NVS status.
- Main dashboard layout remains locked and unchanged.

## Controls
- Tap aquarium: feed fish.
- Hold screen for 2 seconds: open settings.
- Aquarium settings: fish 1–10, bubbles 0–10, plants 0–10, plant length 1–10, animation 0–3, Weather FX on/off, rare-event frequency 0–3.

## Required libraries
- TFT_eSPI
- XPT2046_Touchscreen by Paul Stoffregen
- ArduinoJson
- Adafruit BME280 / BMP280 libraries (and DHT library if enabled)

Copy/edit `Secrets.h` with Wi-Fi credentials before compiling.
