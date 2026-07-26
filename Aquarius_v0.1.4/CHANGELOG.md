# Aquarius Changelog

## v0.1.4 — Special Events

### Added
- Dedicated rarity-aware special-event scheduler inside the Aquarium Engine.
- Event rarity tiers: Uncommon, Rare, Very Rare and Legendary.
- Event cooldowns and randomised gaps so visitors do not appear as a predictable loop.
- Recent-event memory to reduce repeated species and consecutive submarine appearances.
- Time-of-day and weather eligibility rules for selected visitors.
- New pufferfish visitor with periodic inflation animation.
- New seahorse visitor with slow vertical drifting behaviour.
- New ultra-rare legendary whale visitor with slow movement and occasional blowhole spout.
- Variable event durations and speeds by visitor type.

### Changed
- Existing koi, jellyfish, angler fish, shark, squid, octopus and submarine are now selected through the new event system.
- Nocturnal visitors are favoured at night rather than being chosen by a simple fixed branch.
- Submarines are prevented from occurring twice consecutively.
- Dramatic visitors such as shark and octopus are less common than ordinary special visitors.

### Preserved
- Locked v0.1.1 dashboard layout.
- v0.1.2 fish personalities and feeding behaviour.
- v0.1.3 Weather Ecology and gradual day/night/weather transitions.
- Tap-to-feed touchscreen interaction.
