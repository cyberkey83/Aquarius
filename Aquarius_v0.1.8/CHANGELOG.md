# Changelog

## v0.1.8 — Environment & Atmosphere

- sunrise/sunset-driven sun arc across the daytime sky;
- sunset/sunrise-driven moon arc at night;
- stronger coupling between live weather and aquarium behaviour;
- wind direction now fetched from Open-Meteo;
- wind direction biases plant lean, bubble drift, surface movement, rain angle and snow drift;
- strong-wind gust streaks;
- rain/snow/storm/fog effects retained and refined;
- resident fish shelter/deepen in poor weather through the ecology model;
- shark visitor retained and now causes nearby resident fish to scatter;
- new rarer hammerhead shark visitor with its own silhouette and predator response;
- firmware/About version updated to v0.1.8.

## v0.1.7 — Aquarium Life
- Added persistent plant-length control (1–10), separate from plant density.
- Added subtle neighbour separation and schooling behaviour for resident fish.
- Improved bubble grouping, drift and micro-bubble detail.
- Refined crab timing and varied its exploration distance.
- Improved squid/octopus animation and visual detail.
- Fixed boot fish dorsal-fin draw order/clipping.
- Expanded About screen with SSID, IP address and MAC address.
- Preserved the locked dashboard layout.

## v0.1.6 — Environment & Polish
- Replaced tap-to-cycle settings values with dedicated `<` / `>` decrease/increase controls.
- Increased Settings long-press threshold to 2 seconds.
- Added animated boot fish that swims across the startup screen.
- Added changing startup status messages for display, sensor, Wi-Fi, time and weather initialisation.
- Added `cyberkey83` and `github.com/cyberkey83/Aquarius` to the boot screen.
- Added a small whimsical crab cave in the lower-right aquarium scenery.
- Expanded crab behaviour: hiding, peeking, emerging, exploring, waving and returning home.
- Preserved the locked dashboard layout.

## v0.1.5 — Settings
- Resident fish setting supports 1–10 simultaneous fish.
- Bubble and plant density controls support 0–10.
- Added permanent animated crab on the seabed.
- Added long-press touchscreen settings interface.
- Added persistent ESP32 Preferences/NVS storage.
- Aquarium controls: fish count, bubbles, plants, animation, weather FX and rare-event frequency.
- System controls: brightness, night brightness, auto-dim, clock format and temperature units.
- Added About page with firmware, Wi-Fi, sensor and location status.
