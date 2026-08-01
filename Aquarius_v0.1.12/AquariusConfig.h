#pragma once

// ---------------- Indoor sensors ----------------
#define AQUARIUS_ENABLE_DHT 0
#define AQUARIUS_DHT_PIN 16
#define AQUARIUS_SENSOR_INTERVAL_MS 15000UL  // fallback; runtime value is configurable

// ---------------- Wi-Fi ----------------
#define AQUARIUS_WIFI_CONNECT_TIMEOUT_MS 15000UL
#define AQUARIUS_WIFI_RETRY_INTERVAL_MS 30000UL

// ---------------- Clock ----------------
#define AQUARIUS_CLOCK_INTERVAL_MS 1000UL
#define AQUARIUS_TZ "GMT0BST,M3.5.0/1,M10.5.0"
#define AQUARIUS_NTP_1 "pool.ntp.org"
#define AQUARIUS_NTP_2 "time.google.com"
#define AQUARIUS_NTP_3 "time.cloudflare.com"

// ---------------- Outdoor weather ----------------
#define AQUARIUS_LOCATION_NAME "NEWPORT"
#define AQUARIUS_LATITUDE 51.5842
#define AQUARIUS_LONGITUDE -2.9977
#define AQUARIUS_WEATHER_UPDATE_INTERVAL_MS 900000UL
#define AQUARIUS_WEATHER_RETRY_INTERVAL_MS 60000UL
#define AQUARIUS_WEATHER_HTTP_TIMEOUT_MS 10000UL

// ---------------- Pressure trend ----------------
#define AQUARIUS_PRESSURE_SAMPLE_INTERVAL_MS 300000UL
#define AQUARIUS_PRESSURE_TREND_THRESHOLD_HPA 0.5F
#define AQUARIUS_PRESSURE_HISTORY_SIZE 7

// ---------------- Touch controller (XPT2046) ----------------
// Standard ESP32-2432S028R / CYD touch wiring.
#define AQUARIUS_TOUCH_CLK_PIN 25
#define AQUARIUS_TOUCH_MISO_PIN 39
#define AQUARIUS_TOUCH_MOSI_PIN 32
#define AQUARIUS_TOUCH_CS_PIN 33
#define AQUARIUS_TOUCH_IRQ_PIN 36

// Raw calibration values. Adjust these four values if the tap position is
// noticeably offset on a particular CYD revision.
#define AQUARIUS_TOUCH_MIN_X 200
#define AQUARIUS_TOUCH_MAX_X 3700
#define AQUARIUS_TOUCH_MIN_Y 240
#define AQUARIUS_TOUCH_MAX_Y 3800
#define AQUARIUS_TOUCH_MIN_PRESSURE 180
#define AQUARIUS_TOUCH_DEBOUNCE_MS 350UL

// ---------------- Settings / UI ----------------
#define AQUARIUS_LONG_PRESS_MS 2000UL
#define AQUARIUS_SETTINGS_NAMESPACE "aquarius"
#define AQUARIUS_DEFAULT_BRIGHTNESS 220
#define AQUARIUS_DEFAULT_NIGHT_BRIGHTNESS 90
#define AQUARIUS_DEFAULT_DIM_MINUTES 10
