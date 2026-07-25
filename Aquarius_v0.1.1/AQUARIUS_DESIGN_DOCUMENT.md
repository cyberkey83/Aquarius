# Aquarius — Product and Aquarium Engine Design Document

**Project:** Aquarius  
**Target hardware:** ESP32-2432S028R “Cheap Yellow Display”  
**Document version:** 1.0  
**Firmware baseline:** v0.1.1  
**Licence:** GPL-3.0

## 1. Product vision

Aquarius is a **living desktop companion** that combines an immediately readable clock and environmental dashboard with a small aquarium that behaves like a persistent world.

It is not merely a weather station with decorative fish. Real data—time, indoor climate, outdoor weather, pressure and seasonal context—should influence the aquarium’s appearance and behaviour. The dashboard remains readable at a glance while the aquarium rewards longer observation with activity, rare visitors and interaction.

### Core design rule

> Information is the foreground; life is the atmosphere.

The display should be understood in roughly two seconds. Animation must never make the clock or environmental values difficult to read.

## 2. Current screen hierarchy

1. **Clock** — primary visual element.
2. **Indoor and outdoor temperature** — secondary visual elements.
3. **Humidity, pressure and condition** — supporting information.
4. **Rotating footer** — pressure trend, sunrise, sunset, feeding hint and sync state.
5. **Aquarium Engine** — persistent activity behind and around the information.

The v0.0.9 layout is the visual baseline. Future releases should refine it rather than repeatedly redesign it.

## 3. System architecture

```text
Aquarius application
├── IndoorSensor
├── WeatherService
├── TimeService
├── PressureTrend
├── DisplayManager
└── AquariumEngine
    ├── Fish school
    ├── Behaviour controller
    ├── Feeding system
    ├── Timed event scheduler
    ├── Weather effects
    ├── Time-of-day behaviour
    ├── Plants and bubbles
    └── Special visitors
```

`DisplayManager` owns the visual composition. `AquariumEngine` owns aquarium state and behaviour. Sensor and network services should not contain drawing code.

## 4. Aquarium Engine principles

### Persistent state

Fish should have their own position, direction, speed, size and animation phase rather than being recreated from a simple time formula every frame.

### Behaviour before decoration

Weather and interaction should alter behaviour as well as colour:

- fish speed and depth;
- bubble frequency;
- surface movement;
- particle effects;
- special visitor eligibility;
- feeding response.

### Calm default state

Normal activity should remain subtle. Large or unusual events should be uncommon enough to feel special.

### Non-blocking operation

No animation or event may use long `delay()` calls. All movement and scheduling must be based on `millis()` so Wi-Fi, sensors and clock updates continue normally.

## 5. Environmental behaviour model

### Clear weather

- brighter blue water;
- slightly faster fish;
- more bubbles;
- fish use the middle and upper regions more often.

### Cloud or fog

- muted water colour;
- normal or slightly reduced activity;
- softer contrast.

### Rain

- darker water;
- rain-like suspended particles;
- fish slow down and prefer greater depth;
- fewer bubbles.

### Storm

- darkest water state;
- stronger surface motion;
- fish shelter near the bottom;
- occasional subtle lightning flash;
- special surface visitors become less likely.

### Snow

- cool, pale water tint;
- drifting light particles;
- slow movement;
- fish remain deeper.

### Night

- dim navy palette;
- normal fish slow down;
- nocturnal visitors become possible;
- future versions may include sleeping/resting states.

### Sunrise and sunset — planned

The aquarium should eventually transition gradually rather than changing at a fixed hour. Open-Meteo sunrise and sunset values can define a transition window:

- 30 minutes before sunrise to 30 minutes after sunrise;
- 30 minutes before sunset to 45 minutes after sunset.

## 6. Fish and creature system

### Common fish

The normal school supplies continuous low-level movement. Each fish has:

- individual speed;
- direction;
- preferred vertical offset;
- size;
- colour;
- movement phase.

### Rare visitors

Rare visitors appear at scheduled intervals rather than every loop. Initial visitor set:

- koi;
- jellyfish;
- angler fish;
- small shark;
- nocturnal glowing fish.

Future possibilities:

- octopus;
- turtle;
- ray;
- whale silhouette;
- skeleton fish;
- seasonal variants.

### Submarines and objects

A small submarine can cross the lower tank as an uncommon timed event. Later objects may include:

- diver;
- treasure chest animation;
- research probe;
- yellow submarine variant;
- message in a bottle.

## 7. Interaction model

### Feeding

v0.1.1 uses the CYD’s **touchscreen (GPIO 0)** to feed the fish.

On feeding:

1. pellets fall through the water;
2. common fish accelerate;
3. fish move toward the upper feeding region;
4. the behaviour lasts several seconds;
5. fish gradually return to weather-driven behaviour.

Touch-screen feeding is planned after touch calibration and hardware configuration are standardised. A tap near the water surface should eventually place food at the tapped X coordinate.

### Future interactions

- tap a fish for its name/species;
- hold to open aquarium status;
- swipe between dashboard and forecast screens;
- tap decorations to reveal Easter eggs;
- configurable quiet mode.

## 8. Timed and calendar events

### Timed events

Events need three properties:

- eligibility conditions;
- minimum interval and random jitter;
- duration.

Random jitter prevents visitors from becoming predictable. Events should never overlap unless deliberately designed to do so.

### Time-specific Easter eggs — planned

Examples under consideration:

- ghost fish at 03:33;
- special event on Friday the 13th;
- frog or leap-fish on 29 February;
- dawn and dusk species;
- New Year event at midnight.

### Seasonal aquarium — planned

- Halloween pumpkin or ghost fish;
- Christmas decoration and festive fish;
- Valentine heart bubbles;
- April Fool rubber duck.

Seasonal additions must be small and optional rather than overwhelming the dashboard.

## 9. Growth, persistence and unlocks — planned

Aquarius may eventually store aquarium progress in ESP32 Preferences/NVS.

Possible persistent values:

- first-run timestamp;
- total runtime;
- number of feedings;
- visitor sightings;
- unlocked fish and decorations;
- longest online period;
- weather-event counts.

Possible unlocks:

- one week runtime: new plant;
- one month runtime: starfish;
- 100 feedings: koi;
- 100 days runtime: golden fish;
- one year runtime: ancient turtle.

Persistence should be written sparingly to avoid unnecessary flash wear.

## 10. Performance and technical constraints

- Target: approximately 10 frames per second.
- Rendering uses one TFT sprite to reduce flicker.
- Avoid dynamic allocation during the main loop.
- Keep entity counts deliberately small for ESP32 memory and drawing performance.
- All network requests remain outside the animation engine.
- Aquarium update and draw operations must remain deterministic and non-blocking.
- Future persistence must batch writes.

## 11. Release roadmap

### v0.1.1 — Aquarium Engine foundation

- preserve the successful v0.0.9 dashboard;
- move pressure into the Indoor column;
- persistent four-fish school;
- weather-driven activity and preferred depth;
- BOOT-button feeding interaction;
- food particles and feeding response;
- timed rare fish;
- timed submarine;
- nocturnal visitor;
- rain, snow and storm effects.

### v0.1.x — Engine refinement

- tune sprite appearance and movement;
- improve event scheduling;
- use actual sunrise and sunset for lighting transitions;
- add explicit calm/rest states;
- add a debug/event test mode.

### v0.2.0 — Touch and multiple screens

- calibrated touch input;
- tap-to-feed at a chosen location;
- forecast screen;
- indoor trend screen;
- system/about screen;
- swipe navigation.

### v0.3.0 — Persistence and aquarium growth

- NVS save data;
- unlock system;
- aquarium age;
- decorations and sightings log;
- first-run onboarding.

### v0.4.0 — Themes and seasonal events

- Marine, Terminal, Industrial and Night themes;
- calendar-driven events;
- optional seasonal decorations.

### v1.0.0 — Stable desktop companion

- reliable installation and configuration;
- stable touch controls;
- persistent aquarium progression;
- polished animations;
- documented extension points;
- tested upgrade path.

## 12. Definition of success

Aquarius succeeds when it is:

- useful from across a desk;
- calm enough to leave running all day;
- interesting enough to glance at repeatedly;
- surprising without becoming distracting;
- modular enough for contributors to add creatures, events and data sources;
- recognisably Aquarius rather than a generic weather dashboard.


### Touch feeding

The standard interaction is now a screen tap rather than a hardware button. A tap triggers a feeding event, places food near the selected horizontal position, and temporarily changes fish steering and speed so the school converges on the food.

### Cephalopod visitors

The rare-event system includes two behaviourally distinct cephalopods:

- **Squid:** a fast, pulsing mid-water visitor that crosses the tank in short bursts.
- **Octopus:** a slow seabed visitor with independently animated tentacles.

They remain occasional visitors so their appearance retains a sense of discovery.
