## v0.1.14
- Confined rain, snow, fog, strong-wind streaks and lightning to the sky above the water surface.
- Retained rain-impact splashes at the surface and underwater ecological reactions.
- Demo weather scenes now use the corrected atmospheric effect region.

## v0.1.11
- Added hidden full showcase/demo mode for release testing.
- Manual and automatic scene navigation.
- Covers weather, celestial states, all special visitors and calendar events.

# AtmoQuarium changelog

## v0.1.10 — Night Life & Diagnostics

- Added progressive sky atmosphere driven by daylight level.
- Added stable star field that fades in after sunset and fades before sunrise.
- Added a night-only legendary retro swamp-creature event with glowing eyes.
- Extended plant length setting from 1..10 to 1..15.
- Added a wind-responsive plant/anemone growing on the crab hut.
- Added hidden live diagnostics screen: CPU frequency, render load, heap statistics,
  sketch/free flash space, uptime, Wi-Fi RSSI, sensor polling and weather age.
- Diagnostics are accessed by holding the ABOUT title/header for four seconds.
- Indoor sensor polling is now configurable: 1, 5, 10, 15, 30 or 60 seconds.
- Default indoor sensor polling changed to 15 seconds.
- Added Wi-Fi settings page with network scan, SSID selection and on-screen password keyboard.
- Wi-Fi credentials selected in the menu are stored in NVS and override Secrets.h.
- Secrets.h remains the fallback when no saved Wi-Fi network has been configured.
- Firmware, boot and About version updated to v0.1.10.

## v0.1.8 — Environment & Atmosphere

- Added sun/moon tracking using actual sunrise and sunset.
- Added wind-responsive aquarium effects.
- Added shark and hammerhead visitors.

## v0.1.10 — Calendar Events & Wi-Fi Recovery

- Wi-Fi credentials are now tested before being written to NVS.
- Failed Wi-Fi passwords are not saved; AtmoQuarium restores the previous known-good credentials (or `Secrets.h` fallback).
- Added a visible boxed Back control on the Wi-Fi settings page.
- Added `FORGET` to clear only saved Wi-Fi credentials and fall back to `Secrets.h`.
- Added calendar/time Easter eggs:
  - 03:33 Ghost Hour ghost fish.
  - Friday the 13th recurring skeleton-fish / cursed-water window.
  - Halloween pumpkin and ghost-fish visits.
  - Christmas festive tank lights.
  - Valentine heart bubbles.
  - April Fool rubber duck.
  - 29 February Leap Fish.
  - New Year sparkles just after midnight.
  - short dawn/dusk ray visitor windows based on live sunrise/sunset times.


## v0.1.14
- Boot screen visual cleanup.
- Transparent sensor values.
- Backlight dimming rework and diagnostics.
- Calm-weather travelling surface wave.
