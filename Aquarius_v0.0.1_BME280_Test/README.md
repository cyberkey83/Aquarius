# Aquarius v0.0.1 — BME280/BMP280 hardware test

This is the first firmware milestone for **Aquarius**, an animated ASCII aquarium desk companion for the ESP32 Cheap Yellow Display.

This build deliberately uses the Serial Monitor only. Its purpose is to verify the sensor, wiring and I²C pins before display and aquarium code are added.

## Wiring

Use the CYD connector labelled:

```text
GND
IO22
IO27
3.3V
```

Connect:

| CYD | GY-BME/P280 module |
|---|---|
| 3.3V | VCC |
| GND | GND |
| IO22 | SDA |
| IO27 | SCL |

Initially leave `CSB` and `SDO` disconnected.

The firmware checks both normal I²C addresses, `0x76` and `0x77`.

If no sensor is detected:

- Connect `CSB` to `3.3V` to force I²C mode.
- Connect `SDO` to `GND` for address `0x76`, or to `3.3V` for `0x77`.

Do not power the module from 5 V for this test.

## What the test detects

The purple `GY-BME/P280` boards are sometimes sold with either of two Bosch chips:

- **BME280:** temperature, humidity and pressure
- **BMP280:** temperature and pressure only

The firmware reads the chip ID and reports which one is actually installed.

## PlatformIO instructions

1. Install PlatformIO.
2. Open this entire folder as a PlatformIO project.
3. Connect the CYD with a data-capable USB cable.
4. Click **Upload**.
5. Open the Serial Monitor at **115200 baud**.

Useful PlatformIO terminal commands:

```bash
pio run
pio run --target upload
pio device monitor
```

## Arduino IDE instructions

1. Open `arduino/Aquarius_BME280_Test/Aquarius_BME280_Test.ino`.
2. Install these libraries from Library Manager:
   - Adafruit Unified Sensor
   - Adafruit BME280 Library
   - Adafruit BMP280 Library
3. Select an ESP32 board. `ESP32 Dev Module` is suitable for this sensor-only test.
4. Select the CYD serial port.
5. Upload.
6. Open Serial Monitor at **115200 baud**.

## Expected output

For a genuine BME280:

```text
AQUARIUS v0.0.1
Scanning I2C bus...
Device found at 0x76
Device at 0x76 reports chip ID 0x60
Type: BME280

Temperature : 21.64 C
Humidity    : 51.82 %RH
Pressure    : 1014.37 hPa
```

A BMP280 normally reports chip ID `0x58` and will show humidity as unavailable.

## Next milestone

Once this test works, v0.0.2 will draw the sensor readings on the CYD display.
