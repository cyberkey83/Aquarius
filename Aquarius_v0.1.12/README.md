# Aquarius v0.1.10 — Night Life & Diagnostics

Arduino IDE build for the original ILI9341 ESP32-2432S028R CYD used by the Aquarius project.

## Highlights

- Time-of-day sky tint plus progressive stars at night.
- Existing sun/moon arcs remain synced to sunrise and sunset.
- Night-only legendary retro swamp-creature visitor.
- Plant length range increased to 1–15 and a small plant added to the crab hut.
- Configurable indoor sensor polling: 1 / 5 / 10 / 15 / 30 / 60 seconds (15 s default).
- New Wi-Fi settings page: scan networks, select SSID, enter password on the touchscreen,
  connect and save credentials in NVS.
- Hidden diagnostics: from ABOUT, hold the title/header for about four seconds.

## Wi-Fi behaviour

Saved credentials from the Wi-Fi menu take priority. If no saved SSID exists Aquarius falls
back to `AQUARIUS_WIFI_SSID` and `AQUARIUS_WIFI_PASSWORD` in `Secrets.h`.

## Diagnostics

The developer screen reports CPU frequency, approximate render-frame load, free/minimum heap,
largest allocation block, compiled sketch size, free sketch space, uptime, RSSI, sensor poll
interval and weather-data age. "Render load" is the percentage of Aquarius's 100 ms display
frame budget spent drawing/pushing the most recent dashboard frame; it is not a whole-chip CPU
utilisation percentage.

## Arduino IDE

Open `Aquarius_v0.1.10.ino` in Arduino IDE and use the same board/library/TFT_eSPI setup that
worked for v0.1.8.
