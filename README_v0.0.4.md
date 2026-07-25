# Aquarius v0.0.4 — Wi-Fi + NTP clock

v0.0.4 adds network connectivity and a timezone-aware clock to Aquarius.

## New in this build

- Wi-Fi connection
- automatic Wi-Fi reconnect attempts
- SNTP/NTP internet time
- 24-hour clock
- date
- proper UK GMT/BST daylight-saving behaviour
- Wi-Fi status on screen
- existing BME280/BMP280/DHT abstraction retained

## Wi-Fi credentials

Before compiling, open:

`Secrets.h`

Change:

```cpp
#define AQUARIUS_WIFI_SSID "YOUR_WIFI_NAME"
#define AQUARIUS_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

to your real Wi-Fi details.

### IMPORTANT

Do not upload your real `Secrets.h` to GitHub.

A `.gitignore` file is included which excludes `Secrets.h`.

For the public repository, upload `Secrets.example.h` instead.

## UK timezone

Aquarius uses:

```cpp
#define AQUARIUS_TZ "GMT0BST,M3.5.0/1,M10.5.0"
```

This means:

- GMT during winter
- BST during summer
- transition on the final Sunday in March
- transition back on the final Sunday in October

The ESP32's SNTP/timezone facilities then return local time with the correct daylight-saving offset.

## Libraries

No additional Arduino libraries are needed for v0.0.4 beyond those already used in v0.0.3.

WiFi and time support come with the ESP32 Arduino core.

Keep the known-good ILI9341 TFT_eSPI `User_Setup.h` installed.

## Upload

Open:

`Aquarius_v0.0.4.ino`

Settings:

- Board: ESP32 Dev Module
- Upload speed: 460800
- Serial Monitor: 115200

## Expected display

The screen will show:

- Aquarius version
- Wi-Fi connection status
- current 24-hour time
- date
- indoor temperature
- humidity if available
- pressure if available
- detected sensor type

The first NTP synchronisation can take a few seconds.

## GitHub commit recommendation

Do NOT upload your real `Secrets.h`.

Upload:

- Aquarius_v0.0.4.ino
- AquariusConfig.h
- IndoorSensor.h
- IndoorSensor.cpp
- TimeService.h
- TimeService.cpp
- Secrets.example.h
- .gitignore

Suggested commit message:

`Aquarius v0.0.4 - Add Wi-Fi and NTP clock`

## Next milestone

v0.0.5 will add Open-Meteo outdoor temperature and humidity.
