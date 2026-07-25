# Aquarius v0.0.8 JSON Fix

- Corrected Open-Meteo JSON parsing on ESP32.
- The complete HTTP response body is now read before ArduinoJson parses it.
- This avoids `InvalidInput` errors caused by parsing a live chunked HTTP stream.
- Added response-length and response-body diagnostics to Serial Monitor.
- Clock continues to display seconds.

## Sunrise/sunset parsing fix

- Reads Open-Meteo daily sunrise and sunset array values explicitly as strings.
- Avoids an ArduinoJson overload issue that could leave both values blank.
- Adds raw sun-data diagnostics to Serial Monitor.
