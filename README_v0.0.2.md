# Aquarius v0.0.2 — First CYD display build

This milestone adds the first on-screen Aquarius UI to the working v0.0.1 sensor test.

## Libraries

Install in Arduino IDE:

- TFT_eSPI by Bodmer
- Adafruit Unified Sensor
- Adafruit BME280 Library
- Adafruit BMP280 Library

## Sensor wiring

| CYD CN1 | Sensor |
|---|---|
| 3.3V | VCC |
| GND | GND |
| IO22 | SDA |
| IO27 | SCL |

## Configure TFT_eSPI

TFT_eSPI needs the CYD hardware pinout configured at library level.

A ready-made file is included:

`tft_espi/User_Setup.h`

On macOS the library is commonly here:

`~/Documents/Arduino/libraries/TFT_eSPI/`

1. Quit Arduino IDE.
2. Back up the existing `TFT_eSPI/User_Setup.h`.
3. Replace it with the bundled `tft_espi/User_Setup.h`.
4. In `User_Setup_Select.h`, ensure `#include <User_Setup.h>` is enabled.
5. Reopen Arduino IDE.
6. Open `arduino/Aquarius_Display_Test/Aquarius_Display_Test.ino`.
7. Select **ESP32 Dev Module**.
8. Use the upload speed that worked for v0.0.1 (460800).
9. Upload.

The Serial Monitor remains at **115200 baud**.

## Expected display

AQUARIUS v0.0.2 will show:

- sensor type
- temperature
- humidity (`-- %` on BMP280)
- pressure
- I2C address

The common ESP32-2432S028R uses an ILI9341 320×240 display. The bundled TFT_eSPI configuration follows the standard CYD setup also used by the upstream ASCII Aquarium project.
