# Aquarius v0.0.8 — JSON Fix

Complete Arduino build for the ESP32-2432S028R Cheap Yellow Display.

## Fix included

The previous build connected to Open-Meteo successfully but ArduinoJson sometimes returned `InvalidInput` while parsing `HTTPClient::getStream()`. Open-Meteo may use chunked transfer encoding, so this build first reads the complete HTTP body with `http.getString()` and then parses that string.

## Install

1. Extract the ZIP.
2. Open `Secrets.h` and enter your Wi-Fi SSID and password.
3. Open `Aquarius_v0.0.8.ino` in Arduino IDE.
4. Compile and upload using the same board settings used for v0.0.7.
5. Open Serial Monitor at 115200 baud.

A successful update will show:

```text
Weather response   : ... bytes
Weather updated    : ...
```
