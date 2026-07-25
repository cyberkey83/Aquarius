#pragma once

// Sensor configuration
#define AQUARIUS_ENABLE_DHT 0
#define AQUARIUS_DHT_PIN 16
#define AQUARIUS_SENSOR_INTERVAL_MS 2500UL

// Wi-Fi behaviour
#define AQUARIUS_WIFI_CONNECT_TIMEOUT_MS 15000UL
#define AQUARIUS_WIFI_RETRY_INTERVAL_MS 30000UL

// Display update
#define AQUARIUS_CLOCK_INTERVAL_MS 1000UL

// UK timezone:
// GMT in winter, BST (UTC+1) from the last Sunday in March at 01:00
// until the last Sunday in October at 01:00.
#define AQUARIUS_TZ "GMT0BST,M3.5.0/1,M10.5.0"

// NTP servers
#define AQUARIUS_NTP_1 "pool.ntp.org"
#define AQUARIUS_NTP_2 "time.google.com"
#define AQUARIUS_NTP_3 "time.cloudflare.com"
