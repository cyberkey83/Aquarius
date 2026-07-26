# Aquarius — Design Document

## Project idea

Aquarius is an open-source ESP32 desktop companion. Weather, indoor environmental data and time are the information layer; the Aquarium Engine is the personality layer.

The dashboard should remain useful at a glance while the aquarium quietly rewards attention through behaviour, interaction and occasional surprises.

## Locked dashboard principle

The v0.1.1 dashboard layout is considered locked for the current development phase. Minor refinements can wait until the Aquarium Engine and settings architecture are more mature.

Information remains primary. Aquarium animation must never make the clock or environmental readings difficult to read.

## Aquarium Engine architecture

The long-term engine is divided conceptually into:

- Resident fish behaviour
- Feeding / touch interaction
- Weather Ecology
- Special Event Manager
- Environment / decorations
- Settings
- Persistence / progression
- Seasonal and calendar events

## v0.1.4 — Special Event Manager

v0.1.4 introduces a real event-selection layer.

Each event has context rather than being a simple random animation:

1. Aquarius waits for an irregular cooldown.
2. An event rarity is selected.
3. Time, weather and recent event history are considered.
4. An eligible visitor/event is selected.
5. The visitor receives its own movement speed, depth and duration.
6. The event ends and a new irregular cooldown begins.

### Rarity tiers

- Uncommon — most normal special visitors.
- Rare — less frequent creatures and behaviours.
- Very Rare — deliberately infrequent events.
- Legendary — extremely uncommon events intended to surprise long-running installations.

The system should avoid exposing exact legendary probabilities in the normal user interface or eventual wiki spoilers page unless clearly marked.

### Anti-repetition

Aquarius remembers the previous two visitor species when selecting a new one. It also prevents consecutive submarine events. This keeps the aquarium from feeling like a short playlist.

### Context

Examples of contextual eligibility:

- Angler fish and squid receive stronger night-time opportunities.
- Seahorses prefer calmer conditions.
- Some visitors can be suppressed by unsuitable weather.
- Legendary visitors only become eligible after Aquarius has been running for a substantial period.

## Interaction

Normal tap: feed the aquarium at the tap position.

Planned for v0.1.5:

Long press: open Settings.

## v0.1.5 — Settings

Planned aquarium controls:

- Resident fish number
- Bubble density
- Plant density
- Animation intensity
- Weather effects
- Special-event frequency

Planned system controls:

- Brightness
- Night brightness
- Auto-dim timer
- Screen timeout / always-on mode
- 12/24-hour time
- Temperature units
- Wi-Fi/system information
- Sensor information
- Firmware version

Settings should persist across reboot using ESP32 non-volatile storage.

## Longer-term aquarium ideas

- Richer plants, coral and rocks
- Crab / seabed residents
- Hiding places and foreground/background depth
- Fish reacting to one another
- Seasonal visitors and decorations
- Achievements and unlocks
- Aquarium growth over weeks/months
- Persistent companion age
- Rare calendar/time Easter eggs
- User guide/wiki with photographs and annotated screenshots

## Design rule

Aquarius should feel alive without demanding attention. The best events are the ones a user notices unexpectedly while the device continues doing its main job as an elegant desktop information display.


## v0.1.5 Settings architecture
Aquarius now separates persistent user preferences from the Aquarium Engine. A short tap remains an aquarium interaction; a long press opens a full-screen settings UI. Preferences are stored in ESP32 NVS. The dashboard visual layout remains frozen while Aquarium Engine development continues.


## v0.1.5 capacity revision
The resident Aquarium Engine now supports up to ten simultaneous normal fish. Bubble and plant density are exposed as 0–10 user scales. A permanent crab adds independent seabed activity. Rare visitors remain separate from the resident fish limit.
