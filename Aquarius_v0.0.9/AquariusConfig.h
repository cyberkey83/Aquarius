#pragma once

// ---------------- Indoor sensors ----------------
#define AQUARIUS_ENABLE_DHT 0
#define AQUARIUS_DHT_PIN 16
#define AQUARIUS_SENSOR_INTERVAL_MS 2500UL

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
