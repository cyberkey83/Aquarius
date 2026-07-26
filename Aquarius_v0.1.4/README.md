# Aquarius v0.1.4 — Special Events

Aquarius is an open-source ESP32 desktop companion for the ESP32-2432S028R "Cheap Yellow Display". It combines indoor sensing, Open-Meteo weather data, timekeeping and an animated Aquarium Engine.

v0.1.4 deliberately leaves the dashboard layout unchanged and develops the Aquarium Engine's special-event system.

## Special Events

The Aquarium Engine now schedules visitors using rarity, cooldown, recent-event history, time of day and weather conditions rather than choosing from one simple random list.

Current visitors include:

- Koi
- Jellyfish
- Angler fish
- Shark
- Squid
- Octopus
- Pufferfish
- Seahorse
- Submarine
- A deliberately ultra-rare legendary visitor

The exact timing and probability of the rarest event are intentionally not presented on the display. Part of Aquarius' character is occasionally noticing something you have not seen before.

## Event behaviour

- Events occur at irregular intervals rather than on a fixed timer.
- The same species is discouraged from repeating immediately.
- Submarines cannot appear twice consecutively.
- Some nocturnal creatures are more likely after dark.
- Some visitors react to weather eligibility.
- Different visitors have their own speeds, depths and event durations.

## Existing Aquarium Engine features

- Four resident fish personalities: Calm, Explorer, Social and Darting.
- Touchscreen feeding at the tapped location.
- Approach, feeding-frenzy and recovery states.
- Weather-responsive lighting and behaviour.
- Rain, storm, snow and fog ambience.
- Actual sunrise/sunset-based gradual daylight transitions.
- Animated bubbles, plants and water surface.

## Required libraries

Install the same libraries used by v0.1.3, including:

- TFT_eSPI
- ArduinoJson
- Adafruit BME280 / BMP280 support used by the sensor abstraction
- XPT2046_Touchscreen by Paul Stoffregen

## Building

Open:

`Aquarius_v0.1.4.ino`

Enter Wi-Fi details in `Secrets.h`, then use the same ESP32 board, partition and upload settings that worked for v0.1.3.

## Hardware baseline

ESP32-2432S028R V3 / ILI9341 320x240 CYD with optional BMP280, BME280 or DHT22-based indoor sensing.

## Licence

GPL-3.0.
