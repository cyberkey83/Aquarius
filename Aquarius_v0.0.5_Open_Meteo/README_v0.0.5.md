# Aquarius v0.0.5 — Open-Meteo outdoor weather

Aquarius now combines its local indoor sensor with current outdoor conditions from Open-Meteo.

## New features

- Current outdoor temperature
- Current outdoor relative humidity
- Current WMO weather condition
- Indoor/outdoor comparison dashboard
- Automatic weather refresh every 15 minutes
- One-minute retry interval after a failed request
- Existing Wi-Fi, NTP clock and sensor abstraction retained

## New library

Install **ArduinoJson** by Benoit Blanchon using Arduino IDE Library Manager.

The v0.0.5 code uses the ArduinoJson 7 API.

Existing required libraries remain:

- TFT_eSPI
- Adafruit Unified Sensor
- Adafruit BME280 Library
- Adafruit BMP280 Library

## Location

The reference configuration is set to Newport, South Wales:

```cpp
#define AQUARIUS_LOCATION_NAME "NEWPORT"
#define AQUARIUS_LATITUDE 51.5842
#define AQUARIUS_LONGITUDE -2.9977
```

These values are in `AquariusConfig.h` and can be changed for another location.

## Open-Meteo

The firmware requests:

- `temperature_2m`
- `relative_humidity_2m`
- `apparent_temperature`
- `weather_code`
- `is_day`
- `wind_speed_10m`

No API key is required for normal non-commercial use.

The request currently uses plain HTTP because it contains no credentials or private data and avoids storing a changing TLS root certificate on the ESP32. Wi-Fi credentials remain isolated in `Secrets.h`.

## Wi-Fi credentials

Copy:

`Secrets.example.h`

to:

`Secrets.h`

Then enter your Wi-Fi name and password.

`Secrets.h` is excluded by `.gitignore` and must not be committed.

## Upload

Open:

`Aquarius_v0.0.5.ino`

Use:

- Board: ESP32 Dev Module
- Upload speed: 460800
- Serial Monitor: 115200

## Expected display

The screen now has two columns:

- Indoor temperature and humidity
- Outdoor temperature, humidity and weather condition

Indoor barometric pressure remains in the footer.

With the existing BMP280, indoor humidity will still show `-- %`. When a genuine BME280 is connected, the indoor humidity value should appear automatically without a firmware change.

## Suggested commit

`Aquarius v0.0.5 - Add Open-Meteo outdoor weather`

## Next milestone

v0.0.6 is planned to add:

- pressure history and trend arrow
- indoor/outdoor comparison indicators
- sunrise and sunset data
