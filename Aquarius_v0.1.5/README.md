# Aquarius v0.1.5 — Settings

Aquarius is a CYD (ESP32-2432S028R) desktop weather companion with a living Aquarium Engine.

## Controls
- **Tap aquarium:** feed fish at the tap position.
- **Long press (~0.9 s):** open Settings.
- In Settings, tap a row to cycle its value.
- **PAGE >** changes Aquarium / System / About pages.
- **< BACK** saves and returns to the dashboard.

Settings are saved in ESP32 NVS and survive reboot/power loss.

## Required libraries
TFT_eSPI, XPT2046_Touchscreen, ArduinoJson, Adafruit BME280/BMP280/DHT libraries as used by previous Aquarius releases.

Enter Wi-Fi credentials in `Secrets.h` before compiling.


### v0.1.5 aquarium capacity update
- Resident fish population: 1–10 (default 4)
- Bubble density: 0–10
- Plant density: 0–10
- Permanent animated crab on the seabed
