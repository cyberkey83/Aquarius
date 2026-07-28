# Aquarius v0.1.8 — Environment & Atmosphere

Aquarius is an open-source CYD (ESP32-2432S028R) desktop weather companion with a living Aquarium Engine.

## v0.1.8 additions
- Small **sun** tracks an arc from live sunrise to sunset.
- Small **moon** tracks a corresponding night arc from sunset to sunrise.
- Open-Meteo wind direction is now fetched alongside wind speed.
- Wind affects plant lean/sway, bubble drift, surface movement, rain angle and snow drift.
- Strong winds can produce subtle directional gust streaks.
- Rain, snow, fog and storm/lightning effects are refined and remain tied to live weather.
- Fish activity and preferred depth continue to react to weather and daylight.
- Existing shark visitor now causes nearby resident fish to scatter.
- New, rarer **hammerhead shark** visitor with a distinct silhouette.
- Retains all v0.1.7 improvements: plant length control, improved schooling/bubbles/crab/squid/octopus, boot-fin fix, and SSID/IP/MAC on About.
- Main dashboard information hierarchy remains unchanged.

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
