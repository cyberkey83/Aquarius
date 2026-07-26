#include "AquariumEngine.h"

#include <math.h>

namespace {
constexpr int TANK_TOP = 84;
constexpr int TANK_BOTTOM = 201;
constexpr int TANK_LEFT = 7;
constexpr int TANK_RIGHT = 313;
constexpr unsigned long FEED_DURATION_MS = 10000UL;
constexpr unsigned long FEED_APPROACH_MS = 2800UL;
constexpr unsigned long FEED_FRENZY_MS = 6200UL;
constexpr unsigned long MIN_EVENT_GAP_MS = 5UL * 60UL * 1000UL;
constexpr unsigned long EVENT_JITTER_MS = 7UL * 60UL * 1000UL;
constexpr unsigned long LEGENDARY_MIN_UPTIME_MS = 6UL * 60UL * 60UL * 1000UL;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) |
                               ((g & 0xFC) << 3) |
                               (b >> 3));
}

uint16_t blend565(uint16_t a, uint16_t b, uint8_t amount) {
  const int ar = (a >> 11) & 0x1F;
  const int ag = (a >> 5) & 0x3F;
  const int ab = a & 0x1F;
  const int br = (b >> 11) & 0x1F;
  const int bg = (b >> 5) & 0x3F;
  const int bb = b & 0x1F;
  const int r = ar + ((br - ar) * amount) / 255;
  const int g = ag + ((bg - ag) * amount) / 255;
  const int bl = ab + ((bb - ab) * amount) / 255;
  return static_cast<uint16_t>((r << 11) | (g << 5) | bl);
}

bool isRainCode(int code) {
  return (code >= 51 && code <= 67) || (code >= 80 && code <= 82);
}

bool isSnowCode(int code) {
  return (code >= 71 && code <= 77) || (code >= 85 && code <= 86);
}

bool isStormCode(int code) {
  return code >= 95;
}

bool isFogCode(int code) {
  return code == 45 || code == 48;
}

int parseClockMinutes(const String& text) {
  if (text.length() < 5) return -1;
  const int colon = text.indexOf(':');
  if (colon < 1 || colon + 2 >= static_cast<int>(text.length())) return -1;
  const int hour = text.substring(0, colon).toInt();
  const int minute = text.substring(colon + 1, colon + 3).toInt();
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
  return hour * 60 + minute;
}

int parseClockMinutes(const char* text) {
  if (!text || strlen(text) < 5) return -1;
  const int hour = (text[0] - '0') * 10 + (text[1] - '0');
  const int minute = (text[3] - '0') * 10 + (text[4] - '0');
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
  return hour * 60 + minute;
}

float approachFloat(float current, float target, float alpha) {
  if (alpha < 0.0f) alpha = 0.0f;
  if (alpha > 1.0f) alpha = 1.0f;
  return current + (target - current) * alpha;
}

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}
}  // namespace

AquariumEngine::AquariumEngine(TFT_eSprite& canvas) : canvas_(canvas) {}

void AquariumEngine::begin(unsigned long nowMs) {
  randomState_ ^= micros();
  const uint16_t colours[MAX_RESIDENT_FISH] = {
      TFT_CYAN, TFT_GREEN, TFT_ORANGE, TFT_MAGENTA, TFT_YELLOW,
      TFT_BLUE, TFT_RED, TFT_WHITE, TFT_ORANGE, TFT_CYAN};
  const Personality personalities[MAX_RESIDENT_FISH] = {
      Personality::Calm, Personality::Explorer, Personality::Social,
      Personality::Darting, Personality::Calm, Personality::Explorer,
      Personality::Social, Personality::Darting, Personality::Explorer,
      Personality::Calm};
  const float depths[MAX_RESIDENT_FISH] = {
      116.0f, 148.0f, 170.0f, 136.0f, 158.0f,
      126.0f, 178.0f, 145.0f, 166.0f, 132.0f};

  for (uint8_t i = 0; i < MAX_RESIDENT_FISH; ++i) {
    Fish& fish = fish_[i];
    // Spread larger populations across the tank instead of spawning a single clump.
    fish.x = 18.0f + static_cast<float>((i * 61) % 287);
    fish.y = depths[i];
    fish.vx = 0.0f;
    fish.baseSpeed = 8.0f + static_cast<float>((i * 7) % 13) * 0.7f;
    fish.cruiseY = depths[i];
    fish.targetY = depths[i];
    fish.direction = (i % 2 == 0) ? 1 : -1;
    fish.size = (i == 3 || i == 7) ? 2 : 1;
    fish.colour = colours[i];
    fish.phase = nextRandom();
    fish.personality = personalities[i];
    fish.state = FishState::Cruise;
    fish.nextDecisionMs = nowMs + 1500UL + (nextRandom() % 4500UL);
    fish.stateUntilMs = 0;
  }

  lastUpdateMs_ = nowMs;
  bootMs_ = nowMs;
  scheduleNextEvent(nowMs);
  initialised_ = true;
}

uint32_t AquariumEngine::nextRandom() {
  randomState_ = randomState_ * 1664525u + 1013904223u;
  return randomState_;
}

float AquariumEngine::randomUnit() {
  return static_cast<float>(nextRandom() & 0xFFFFu) / 65535.0f;
}

bool AquariumEngine::isNight(const ClockReading& clock) const {
  if (!clock.valid || clock.timeText.length() < 2) return false;
  const int hour = clock.timeText.substring(0, 2).toInt();
  return hour < 6 || hour >= 21;
}

float AquariumEngine::activityFactor(
    const ClockReading& clock,
    const OutdoorReading& outdoor) const {
  (void)clock;
  (void)outdoor;
  return ecologyActivity_;
}

float AquariumEngine::preferredDepth(const OutdoorReading& outdoor) const {
  (void)outdoor;
  return ecologyDepth_;
}

float AquariumEngine::daylightFactor(
    const ClockReading& clock,
    const OutdoorReading& outdoor) const {
  if (!clock.valid) return 1.0f;

  const int nowMinutes = parseClockMinutes(clock.timeText);
  if (nowMinutes < 0) return isNight(clock) ? 0.08f : 1.0f;

  int sunrise = -1;
  int sunset = -1;
  if (outdoor.valid) {
    sunrise = parseClockMinutes(outdoor.sunriseText);
    sunset = parseClockMinutes(outdoor.sunsetText);
  }

  // Fall back to a simple 06:00 / 21:00 day when sunrise data is missing.
  if (sunrise < 0 || sunset < 0 || sunset <= sunrise) {
    sunrise = 6 * 60;
    sunset = 21 * 60;
  }

  constexpr float NIGHT_LEVEL = 0.08f;
  constexpr int TWILIGHT_MINUTES = 45;
  const int dawnStart = sunrise - TWILIGHT_MINUTES;
  const int dawnEnd = sunrise + TWILIGHT_MINUTES;
  const int duskStart = sunset - TWILIGHT_MINUTES;
  const int duskEnd = sunset + TWILIGHT_MINUTES;

  if (nowMinutes <= dawnStart || nowMinutes >= duskEnd) return NIGHT_LEVEL;
  if (nowMinutes >= dawnEnd && nowMinutes <= duskStart) return 1.0f;
  if (nowMinutes < dawnEnd) {
    const float t = static_cast<float>(nowMinutes - dawnStart) /
                    static_cast<float>(dawnEnd - dawnStart);
    return NIGHT_LEVEL + (1.0f - NIGHT_LEVEL) * clampFloat(t, 0.0f, 1.0f);
  }

  const float t = static_cast<float>(nowMinutes - duskStart) /
                  static_cast<float>(duskEnd - duskStart);
  return 1.0f - (1.0f - NIGHT_LEVEL) * clampFloat(t, 0.0f, 1.0f);
}

void AquariumEngine::updateEcology(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  float dt = (nowMs - lastUpdateMs_) / 1000.0f;
  if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

  const float daylight = daylightFactor(clock, outdoor);
  float rain = 0.0f;
  float snow = 0.0f;
  float storm = 0.0f;
  float fog = 0.0f;
  float cloud = 0.0f;
  float wind = 0.0f;

  if (outdoor.valid) {
    const int code = outdoor.weatherCode;
    if (code == 1) cloud = 0.18f;
    if (code == 2) cloud = 0.55f;
    if (code == 3) cloud = 1.0f;
    if (isFogCode(code)) {
      fog = 1.0f;
      cloud = 0.75f;
    }
    if (isRainCode(code)) {
      rain = (code >= 80) ? 1.0f : 0.78f;
      cloud = 0.9f;
    }
    if (isSnowCode(code)) {
      snow = 1.0f;
      cloud = 0.85f;
    }
    if (isStormCode(code)) {
      storm = 1.0f;
      rain = 0.9f;
      cloud = 1.0f;
    }
    if (!isnan(outdoor.windSpeedKmh)) {
      wind = clampFloat(outdoor.windSpeedKmh / 45.0f, 0.0f, 1.0f);
    }
  }

  float activity = 0.50f + daylight * 0.53f;
  if (outdoor.valid && outdoor.weatherCode <= 1) activity += 0.15f;
  activity -= cloud * 0.08f;
  activity -= rain * 0.18f;
  activity -= snow * 0.27f;
  activity -= fog * 0.17f;
  activity -= storm * 0.28f;
  activity = clampFloat(activity, 0.38f, 1.22f);

  float depth = 142.0f;
  depth += (1.0f - daylight) * 5.0f;
  depth += rain * 14.0f + snow * 20.0f + fog * 5.0f + storm * 18.0f;
  depth = clampFloat(depth, 140.0f, 180.0f);

  float surface = 0.26f + wind * 0.38f + rain * 0.24f + storm * 0.46f;
  surface = clampFloat(surface, 0.20f, 1.0f);

  float bubbleRate = 0.68f + activity * 0.33f;
  if (outdoor.valid && outdoor.weatherCode <= 1) bubbleRate += 0.12f;
  bubbleRate -= storm * 0.12f;
  bubbleRate = clampFloat(bubbleRate, 0.48f, 1.22f);

  // Base weather colour before the day/night lighting pass.
  uint16_t dayColour = rgb565(0, 17, 31);
  if (cloud > 0.7f) dayColour = rgb565(4, 13, 25);
  if (fog > 0.2f) dayColour = rgb565(8, 16, 24);
  if (rain > 0.2f) dayColour = rgb565(1, 9, 25);
  if (snow > 0.2f) dayColour = rgb565(8, 18, 27);
  if (storm > 0.2f) dayColour = rgb565(7, 4, 20);

  const uint16_t nightColour = rgb565(0, 4, 16);
  const uint8_t daylightBlend = static_cast<uint8_t>(clampFloat(daylight, 0.0f, 1.0f) * 255.0f);
  const uint16_t targetBg = blend565(nightColour, dayColour, daylightBlend);
  const float targetR = static_cast<float>(((targetBg >> 11) & 0x1F) * 255) / 31.0f;
  const float targetG = static_cast<float>(((targetBg >> 5) & 0x3F) * 255) / 63.0f;
  const float targetB = static_cast<float>((targetBg & 0x1F) * 255) / 31.0f;

  if (!ecologySeeded_) {
    ecologyDaylight_ = daylight;
    ecologyRain_ = rain;
    ecologySnow_ = snow;
    ecologyStorm_ = storm;
    ecologyFog_ = fog;
    ecologyCloud_ = cloud;
    ecologyWind_ = wind;
    ecologyActivity_ = activity;
    ecologyDepth_ = depth;
    ecologySurface_ = surface;
    ecologyBubbleRate_ = bubbleRate;
    ecologyBgR_ = targetR;
    ecologyBgG_ = targetG;
    ecologyBgB_ = targetB;
    ecologySeeded_ = true;
    return;
  }

  // Weather moods cross-fade over roughly 75 seconds. Daylight follows more
  // quickly because the sunrise/sunset target itself already changes slowly.
  const float weatherAlpha = clampFloat(dt / 75.0f, 0.0f, 1.0f);
  const float lightAlpha = clampFloat(dt / 12.0f, 0.0f, 1.0f);
  ecologyDaylight_ = approachFloat(ecologyDaylight_, daylight, lightAlpha);
  ecologyRain_ = approachFloat(ecologyRain_, rain, weatherAlpha);
  ecologySnow_ = approachFloat(ecologySnow_, snow, weatherAlpha);
  ecologyStorm_ = approachFloat(ecologyStorm_, storm, weatherAlpha);
  ecologyFog_ = approachFloat(ecologyFog_, fog, weatherAlpha);
  ecologyCloud_ = approachFloat(ecologyCloud_, cloud, weatherAlpha);
  ecologyWind_ = approachFloat(ecologyWind_, wind, weatherAlpha);
  ecologyActivity_ = approachFloat(ecologyActivity_, activity, weatherAlpha);
  ecologyDepth_ = approachFloat(ecologyDepth_, depth, weatherAlpha);
  ecologySurface_ = approachFloat(ecologySurface_, surface, weatherAlpha);
  ecologyBubbleRate_ = approachFloat(ecologyBubbleRate_, bubbleRate, weatherAlpha);
  ecologyBgR_ = approachFloat(ecologyBgR_, targetR, weatherAlpha);
  ecologyBgG_ = approachFloat(ecologyBgG_, targetG, weatherAlpha);
  ecologyBgB_ = approachFloat(ecologyBgB_, targetB, weatherAlpha);
}

float AquariumEngine::personalitySpeedFactor(Personality personality) const {
  switch (personality) {
    case Personality::Calm: return 0.78f;
    case Personality::Explorer: return 1.00f;
    case Personality::Social: return 0.92f;
    case Personality::Darting: return 1.30f;
    default: return 1.0f;
  }
}

float AquariumEngine::personalityWander(Personality personality) const {
  switch (personality) {
    case Personality::Calm: return 10.0f;
    case Personality::Explorer: return 28.0f;
    case Personality::Social: return 18.0f;
    case Personality::Darting: return 23.0f;
    default: return 16.0f;
  }
}

void AquariumEngine::scheduleNextEvent(unsigned long nowMs) {
  // Events are deliberately infrequent and irregular. The gap is measured
  // from the end of the previous event so visitors never feel like a loop.
  unsigned long gap = MIN_EVENT_GAP_MS + (nextRandom() % EVENT_JITTER_MS);
  // 0 = very quiet, 1 = quiet, 2 = normal, 3 = lively.
  const float frequencyScale = eventFrequency_ == 0 ? 2.2f :
                               eventFrequency_ == 1 ? 1.45f :
                               eventFrequency_ == 2 ? 1.0f : 0.62f;
  gap = static_cast<unsigned long>(gap * frequencyScale);
  if (eventRarity_ == EventRarity::VeryRare) gap += 2UL * 60UL * 1000UL;
  if (eventRarity_ == EventRarity::Legendary) gap += 5UL * 60UL * 1000UL;
  nextRareEventMs_ = nowMs + gap;
}

AquariumEngine::EventRarity AquariumEngine::chooseEventRarity(unsigned long nowMs) {
  const uint32_t roll = nextRandom() % 1000u;
  const bool legendaryUnlocked = (nowMs - bootMs_) >= LEGENDARY_MIN_UPTIME_MS;

  // Most events are uncommon/rare. Legendary events stay genuinely special.
  if (legendaryUnlocked && roll < 8u) return EventRarity::Legendary;
  if (roll < 70u) return EventRarity::VeryRare;
  if (roll < 330u) return EventRarity::Rare;
  return EventRarity::Uncommon;
}

bool AquariumEngine::speciesEligible(
    RareSpecies species,
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    EventRarity rarity) const {
  const bool night = isNight(clock);

  if (species == RareSpecies::Whale) {
    return rarity == EventRarity::Legendary;
  }
  if (rarity == EventRarity::Legendary) return false;

  // Nocturnal creatures are possible by day, but are strongly favoured at night.
  if ((species == RareSpecies::AnglerFish || species == RareSpecies::Squid) &&
      !night && rarity == EventRarity::Uncommon) return false;

  // Seahorses favour calmer conditions. Sharks are less likely in storms/snow.
  if (outdoor.valid) {
    if (species == RareSpecies::Seahorse &&
        (isStormCode(outdoor.weatherCode) || isSnowCode(outdoor.weatherCode)))
      return false;
    if (species == RareSpecies::Shark && isSnowCode(outdoor.weatherCode))
      return false;
  }

  return true;
}

AquariumEngine::RareSpecies AquariumEngine::chooseRareSpecies(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    EventRarity rarity) {
  if (rarity == EventRarity::Legendary) return RareSpecies::Whale;

  // Try several candidates while avoiding the two most recent species.
  for (uint8_t attempt = 0; attempt < 20; ++attempt) {
    RareSpecies candidate = static_cast<RareSpecies>(
        nextRandom() % static_cast<uint8_t>(RareSpecies::Whale));
    if (candidate == lastSpecies_ || candidate == previousSpecies_) continue;
    if (!speciesEligible(candidate, clock, outdoor, rarity)) continue;

    // Rarity gates keep the more dramatic animals from becoming routine.
    if (candidate == RareSpecies::Octopus && rarity == EventRarity::Uncommon &&
        (nextRandom() % 3u) != 0u) continue;
    if (candidate == RareSpecies::Shark && rarity == EventRarity::Uncommon &&
        (nextRandom() % 4u) != 0u) continue;
    if (candidate == RareSpecies::Squid && !isNight(clock) &&
        (nextRandom() % 3u) != 0u) continue;

    return candidate;
  }

  // Safe fallback that remains eligible in almost every condition.
  return RareSpecies::Koi;
}

unsigned long AquariumEngine::eventDurationFor(
    RareSpecies species, EventType type) const {
  if (type == EventType::Submarine) return 22000UL;
  if (type == EventType::LegendaryVisitor) return 30000UL;
  switch (species) {
    case RareSpecies::Octopus: return 26000UL;
    case RareSpecies::Jellyfish: return 23000UL;
    case RareSpecies::Seahorse: return 24000UL;
    case RareSpecies::Squid: return 17000UL;
    case RareSpecies::Shark: return 15000UL;
    default: return 19000UL;
  }
}

float AquariumEngine::eventBaseSpeed(RareSpecies species, EventType type) const {
  if (type == EventType::Submarine) return 18.0f;
  if (type == EventType::LegendaryVisitor) return 11.0f;
  switch (species) {
    case RareSpecies::Octopus: return 6.5f;
    case RareSpecies::Jellyfish: return 11.0f;
    case RareSpecies::Seahorse: return 9.0f;
    case RareSpecies::PufferFish: return 15.0f;
    case RareSpecies::Squid: return 24.0f;
    case RareSpecies::Shark: return 31.0f;
    default: return 24.0f;
  }
}

void AquariumEngine::startScheduledEvent(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  eventRarity_ = chooseEventRarity(nowMs);
  const bool night = isNight(clock);

  if (eventRarity_ == EventRarity::Legendary) {
    eventType_ = EventType::LegendaryVisitor;
    rareSpecies_ = RareSpecies::Whale;
  } else {
    const uint32_t roll = nextRandom() % 100u;
    const bool submarineAllowed = lastEventType_ != EventType::Submarine;
    if (submarineAllowed && roll < 18u) {
      eventType_ = EventType::Submarine;
      rareSpecies_ = RareSpecies::Koi;
    } else {
      rareSpecies_ = chooseRareSpecies(clock, outdoor, eventRarity_);
      eventType_ = (night &&
                    (rareSpecies_ == RareSpecies::AnglerFish ||
                     rareSpecies_ == RareSpecies::Squid ||
                     rareSpecies_ == RareSpecies::Jellyfish))
                       ? EventType::NocturnalVisitor
                       : EventType::Visitor;
    }
  }

  eventDirection_ = (nextRandom() & 1u) ? 1 : -1;
  eventX_ = eventDirection_ > 0 ? -48.0f : 368.0f;
  eventY_ = 137.0f;
  eventVelocity_ = 0.0f;
  eventNextActionMs_ = nowMs + 1000UL + (nextRandom() % 2200UL);

  if (eventType_ == EventType::Submarine) eventY_ = 183.0f;
  if (rareSpecies_ == RareSpecies::Octopus) eventY_ = 190.0f;
  if (rareSpecies_ == RareSpecies::Squid) eventY_ = 135.0f;
  if (rareSpecies_ == RareSpecies::Jellyfish) eventY_ = 128.0f;
  if (rareSpecies_ == RareSpecies::Seahorse) eventY_ = 155.0f;
  if (rareSpecies_ == RareSpecies::PufferFish) eventY_ = 145.0f;
  if (rareSpecies_ == RareSpecies::Whale) eventY_ = 150.0f;

  eventStartedMs_ = nowMs;
  eventUntilMs_ = nowMs + eventDurationFor(rareSpecies_, eventType_);

  if (eventType_ != EventType::Submarine) {
    previousSpecies_ = lastSpecies_;
    lastSpecies_ = rareSpecies_;
  }
  lastEventType_ = eventType_;
  scheduleNextEvent(eventUntilMs_);
}

void AquariumEngine::configure(uint8_t fishCount, uint8_t bubbleLevel, uint8_t plantLevel,
                               uint8_t animationLevel, bool weatherEffects, uint8_t eventFrequency) {
  fishCount_ = constrain((int)fishCount, 1, 10);
  bubbleLevel_ = constrain((int)bubbleLevel, 0, 10);
  plantLevel_ = constrain((int)plantLevel, 0, 10);
  animationLevel_ = constrain((int)animationLevel, 0, 3);
  eventFrequency_ = constrain((int)eventFrequency, 0, 3);
  weatherEffectsEnabled_ = weatherEffects;
}

void AquariumEngine::feed(unsigned long nowMs, int screenX, int screenY) {
  feedingStartedMs_ = nowMs;
  feedingUntilMs_ = nowMs + FEED_DURATION_MS;
  feedingX_ = constrain(screenX, TANK_LEFT + 16, TANK_RIGHT - 16);
  feedingY_ = constrain(screenY, 94, 146);

  for (uint8_t i = 0; i < fishCount_; ++i) {
    fish_[i].state = FishState::FeedApproach;
    fish_[i].stateUntilMs = nowMs + FEED_APPROACH_MS;
    fish_[i].nextDecisionMs = feedingUntilMs_ + 800UL + (nextRandom() % 1800UL);
  }
}

bool AquariumEngine::feedingActive(unsigned long nowMs) const {
  return static_cast<long>(feedingUntilMs_ - nowMs) > 0;
}

const char* AquariumEngine::eventLabel(unsigned long nowMs) const {
  if (feedingActive(nowMs)) return "FEEDING TIME";
  if (static_cast<long>(eventUntilMs_ - nowMs) <= 0) return "";
  switch (eventType_) {
    case EventType::Visitor: return "SPECIAL VISITOR";
    case EventType::Submarine: return "SUBMARINE";
    case EventType::NocturnalVisitor: return "NIGHT VISITOR";
    case EventType::LegendaryVisitor: return "LEGENDARY VISITOR";
    default: return "";
  }
}

void AquariumEngine::update(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  if (!initialised_) begin(nowMs);
  updateEcology(nowMs, clock, outdoor);
  updateFish(nowMs, clock, outdoor);
  updateEvent(nowMs, clock, outdoor);
  lastUpdateMs_ = nowMs;
}

void AquariumEngine::chooseFishTarget(
    Fish& fish,
    uint8_t index,
    unsigned long nowMs,
    const OutdoorReading& outdoor) {
  const float weatherDepth = preferredDepth(outdoor);
  const float personalityRange = personalityWander(fish.personality);
  const float randomOffset = (randomUnit() * 2.0f - 1.0f) * personalityRange;

  float base = fish.cruiseY;
  if (outdoor.valid &&
      (isRainCode(outdoor.weatherCode) || isSnowCode(outdoor.weatherCode) ||
       isStormCode(outdoor.weatherCode))) {
    base = weatherDepth + (index - 1.5f) * 7.0f;
  } else {
    base = base * 0.65f + weatherDepth * 0.35f;
  }

  if (fish.personality == Personality::Social) {
    float schoolY = 0.0f;
    for (uint8_t i = 0; i < fishCount_; ++i) schoolY += fish_[i].y;
    schoolY /= static_cast<float>(fishCount_);
    base = base * 0.45f + schoolY * 0.55f;
  }

  fish.targetY = clampFloat(base + randomOffset, 99.0f, 188.0f);

  // Occasional voluntary turn makes cruising feel less like a screen saver.
  const uint32_t turnChance = nextRandom() % 100u;
  const uint32_t threshold =
      fish.personality == Personality::Explorer ? 32u :
      fish.personality == Personality::Darting ? 25u : 16u;
  if (turnChance < threshold) {
    fish.direction *= -1;
    fish.state = FishState::Turn;
    fish.stateUntilMs = nowMs + 350UL;
  } else {
    fish.state = FishState::Cruise;
  }

  unsigned long delayMs = 4200UL + (nextRandom() % 5200UL);
  if (fish.personality == Personality::Calm) delayMs += 2500UL;
  if (fish.personality == Personality::Darting) delayMs /= 2UL;
  fish.nextDecisionMs = nowMs + delayMs;
}

void AquariumEngine::updateFish(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  float dt = (nowMs - lastUpdateMs_) / 1000.0f;
  if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

  const float animationScale = 0.55f + 0.25f * animationLevel_;
  const float activity = activityFactor(clock, outdoor) * animationScale;
  const bool feeding = feedingActive(nowMs);
  const unsigned long feedElapsed = feeding ? nowMs - feedingStartedMs_ : 0UL;

  for (uint8_t i = 0; i < fishCount_; ++i) {
    Fish& fish = fish_[i];

    if (feeding) {
      if (feedElapsed < FEED_APPROACH_MS) {
        fish.state = FishState::FeedApproach;
      } else if (feedElapsed < FEED_FRENZY_MS) {
        fish.state = FishState::FeedFrenzy;
      } else {
        fish.state = FishState::Recover;
      }
    } else if (fish.state == FishState::FeedApproach ||
               fish.state == FishState::FeedFrenzy) {
      fish.state = FishState::Recover;
      fish.stateUntilMs = nowMs + 2200UL;
    } else if (fish.state == FishState::Recover) {
      if (static_cast<long>(nowMs - fish.stateUntilMs) >= 0) {
        fish.state = FishState::Cruise;
        fish.nextDecisionMs = nowMs;
      }
    } else if (static_cast<long>(nowMs - fish.nextDecisionMs) >= 0) {
      chooseFishTarget(fish, i, nowMs, outdoor);
    }

    float targetY = fish.targetY;
    float speedFactor = personalitySpeedFactor(fish.personality);
    float verticalResponse = 0.42f;

    if (feeding) {
      const float dx = static_cast<float>(feedingX_) - fish.x;
      if (fabsf(dx) > 3.0f) fish.direction = dx > 0.0f ? 1 : -1;

      if (fish.state == FishState::FeedApproach) {
        targetY = static_cast<float>(feedingY_) + (i - 1.5f) * 5.0f;
        speedFactor *= 1.75f;
        verticalResponse = 1.75f;
      } else if (fish.state == FishState::FeedFrenzy) {
        const float orbit = sinf((nowMs + fish.phase) / (220.0f + i * 27.0f));
        targetY = static_cast<float>(feedingY_) + orbit * (8.0f + i * 2.0f);
        speedFactor *= 2.15f + (i * 0.08f);
        verticalResponse = 2.4f;
        if (((nowMs + fish.phase) / 750UL) % 2UL == 0UL && fabsf(dx) < 20.0f) {
          fish.direction = ((nowMs + fish.phase) / 750UL) % 4UL < 2UL ? 1 : -1;
        }
      } else {
        targetY = fish.cruiseY * 0.65f + preferredDepth(outdoor) * 0.35f;
        speedFactor *= 1.15f;
        verticalResponse = 0.65f;
      }
    } else if (fish.state == FishState::Recover) {
      targetY = fish.cruiseY * 0.65f + preferredDepth(outdoor) * 0.35f;
      speedFactor *= 0.90f;
      verticalResponse = 0.65f;
    }

    // Darting fish occasionally gets a brief burst even when not feeding.
    if (!feeding && fish.personality == Personality::Darting &&
        ((nowMs + fish.phase) % 9000UL) < 900UL) {
      speedFactor *= 1.55f;
    }

    const float desiredVelocity =
        fish.baseSpeed * activity * speedFactor * fish.direction;
    const float acceleration = feeding ? 7.0f : 3.3f;
    fish.vx += (desiredVelocity - fish.vx) * clampFloat(dt * acceleration, 0.0f, 1.0f);
    fish.x += fish.vx * dt;

    fish.y += (targetY - fish.y) * dt * verticalResponse;
    fish.y += sinf((nowMs + fish.phase) / 720.0f) * 0.10f;

    const float margin = 18.0f + fish.size * 3.0f;
    if (fish.x > TANK_RIGHT + margin) {
      fish.x = TANK_RIGHT + margin;
      fish.direction = -1;
      fish.vx = -fabsf(fish.vx) * 0.55f;
      fish.state = FishState::Turn;
      fish.stateUntilMs = nowMs + 300UL;
    } else if (fish.x < TANK_LEFT - margin) {
      fish.x = TANK_LEFT - margin;
      fish.direction = 1;
      fish.vx = fabsf(fish.vx) * 0.55f;
      fish.state = FishState::Turn;
      fish.stateUntilMs = nowMs + 300UL;
    }

    fish.y = clampFloat(fish.y, 94.0f, 191.0f);
  }
}

void AquariumEngine::updateEvent(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  if (eventType_ == EventType::None &&
      static_cast<long>(nowMs - nextRareEventMs_) >= 0) {
    startScheduledEvent(nowMs, clock, outdoor);
  }
  if (eventType_ == EventType::None) return;
  if (static_cast<long>(nowMs - eventUntilMs_) >= 0) {
    eventType_ = EventType::None;
    return;
  }

  float dt = (nowMs - lastUpdateMs_) / 1000.0f;
  if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

  float targetSpeed = eventBaseSpeed(rareSpecies_, eventType_);

  if (eventType_ != EventType::Submarine) {
    switch (rareSpecies_) {
      case RareSpecies::Octopus:
        // Crawl, pause, then creep again along the seabed.
        if (static_cast<long>(nowMs - eventNextActionMs_) >= 0) {
          eventVelocity_ = (nextRandom() % 4u == 0u) ? 0.0f : targetSpeed;
          eventNextActionMs_ = nowMs + 900UL + (nextRandom() % 1700UL);
        }
        eventY_ = 190.0f + sinf(nowMs / 850.0f) * 1.2f;
        break;

      case RareSpecies::Squid:
        // Pulsed propulsion: calm glide punctuated by sharp darts.
        targetSpeed = ((nowMs / 950UL) % 4UL == 0UL) ? 48.0f : 24.0f;
        eventY_ = 135.0f + sinf(nowMs / 470.0f) * 7.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 5.5f, 0.0f, 1.0f);
        break;

      case RareSpecies::Jellyfish:
        eventY_ = 128.0f + sinf(nowMs / 640.0f) * 8.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 2.0f, 0.0f, 1.0f);
        break;

      case RareSpecies::Seahorse:
        eventY_ = 155.0f + sinf(nowMs / 900.0f) * 12.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 1.6f, 0.0f, 1.0f);
        break;

      case RareSpecies::PufferFish: {
        const bool puffed = ((nowMs - eventStartedMs_) / 1800UL) % 3UL == 1UL;
        targetSpeed *= puffed ? 0.55f : 1.0f;
        eventY_ = 145.0f + sinf(nowMs / 560.0f) * 4.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 2.4f, 0.0f, 1.0f);
        break;
      }

      case RareSpecies::Whale:
        // The legendary visitor should feel heavy and unhurried.
        eventY_ = 150.0f + sinf(nowMs / 1250.0f) * 3.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 1.4f, 0.0f, 1.0f);
        break;

      default:
        eventVelocity_ += (targetSpeed - eventVelocity_) *
                          clampFloat(dt * 3.0f, 0.0f, 1.0f);
        break;
    }
  } else {
    eventVelocity_ += (targetSpeed - eventVelocity_) *
                      clampFloat(dt * 3.0f, 0.0f, 1.0f);
  }

  if (rareSpecies_ == RareSpecies::Octopus &&
      eventType_ != EventType::Submarine &&
      eventVelocity_ <= 0.1f &&
      static_cast<long>(eventNextActionMs_ - nowMs) > 0) {
    return;
  }

  eventX_ += eventVelocity_ * eventDirection_ * dt;
}

uint16_t AquariumEngine::backgroundColour(
    const ClockReading& clock,
    const OutdoorReading& outdoor) const {
  (void)clock;
  (void)outdoor;
  const uint8_t r = static_cast<uint8_t>(clampFloat(ecologyBgR_, 0.0f, 255.0f));
  const uint8_t g = static_cast<uint8_t>(clampFloat(ecologyBgG_, 0.0f, 255.0f));
  const uint8_t b = static_cast<uint8_t>(clampFloat(ecologyBgB_, 0.0f, 255.0f));
  return rgb565(r, g, b);
}

void AquariumEngine::draw(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    unsigned long nowMs,
    uint16_t background) {
  drawWaterSurface(nowMs, background, outdoor);
  drawWeatherEffects(nowMs, background, outdoor);
  drawBubbles(nowMs, background, clock, outdoor);
  drawPlants(nowMs, background);
  drawCrab(nowMs, background);
  drawFood(nowMs, background);
  drawFishSchool(nowMs, background);
  drawSpecialEvent(nowMs, background);
}

void AquariumEngine::drawWaterSurface(
    unsigned long nowMs,
    uint16_t background,
    const OutdoorReading& outdoor) {
  (void)outdoor;
  const uint16_t water = blend565(background, TFT_CYAN,
      static_cast<uint8_t>(80.0f + ecologySurface_ * 95.0f));
  const int speed = static_cast<int>(220.0f - ecologySurface_ * 145.0f);
  const int shift = static_cast<int>((nowMs / max(speed, 60)) % 8UL);
  const int amplitude = ecologySurface_ > 0.72f ? 3 : (ecologySurface_ > 0.43f ? 2 : 1);
  for (int x = -8; x < 320; x += 16) {
    const int x0 = x + shift;
    canvas_.drawFastHLine(x0, 82, 7, water);
    canvas_.drawFastHLine(x0 + 8, 82 + amplitude, 7, water);
  }

  // Rain and wind occasionally create tiny surface splashes.
  const int splashCount = static_cast<int>(ecologyRain_ * 6.0f + ecologyWind_ * 2.0f);
  for (int i = 0; i < splashCount; ++i) {
    const int x = 18 + ((i * 53 + static_cast<int>(nowMs / 95UL)) % 284);
    const int h = 1 + ((i + static_cast<int>(nowMs / 300UL)) % 2);
    canvas_.drawLine(x - 2, 84, x, 84 - h, water);
    canvas_.drawLine(x, 84 - h, x + 2, 84, water);
  }
}

void AquariumEngine::drawBubbles(
    unsigned long nowMs,
    uint16_t background,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  const uint16_t bubble = blend565(background, TFT_CYAN, 145);
  (void)clock;
  (void)outdoor;
  float rate = ecologyBubbleRate_;
  if (rate < 0.45f) rate = 0.45f;
  // Density is a user-facing 0..10 scale. Each step adds two independent
  // bubble streams; level 0 is genuinely bubble-free.
  const unsigned long period = static_cast<unsigned long>(6200.0f / rate);
  const uint8_t bubbleCount = bubbleLevel_ * 2;
  for (uint8_t i = 0; i < bubbleCount; ++i) {
    const int baseX = TANK_LEFT + 9 + ((i * 47 + (i * i * 11)) % (TANK_RIGHT - TANK_LEFT - 18));
    const uint32_t offset = static_cast<uint32_t>(i) * 733UL + static_cast<uint32_t>(i % 5) * 1271UL;
    const unsigned long phase = (nowMs + offset) % period;
    const int y = TANK_BOTTOM - static_cast<int>((phase * (TANK_BOTTOM - 88)) / period);
    const int drift = static_cast<int>((phase / 500UL + i) % 5UL) - 2;
    const int radius = (i % 4 == 0) ? 2 : 1;
    canvas_.drawCircle(baseX + drift, y, radius, bubble);
  }
}

void AquariumEngine::drawPlants(unsigned long nowMs, uint16_t background) {
  if (plantLevel_ == 0) return;
  const uint16_t plant = blend565(background, TFT_GREEN, 155);
  const int swayRange = 2 + static_cast<int>(ecologySurface_ * 2.5f);
  const int phase = static_cast<int>((nowMs / 420UL) % static_cast<unsigned long>(swayRange * 2 + 1));
  const int sway = phase - swayRange;

  // 0..10 maps directly to the number of plant clusters. Clusters alternate
  // across the seabed and vary in height so high density still looks organic.
  for (uint8_t i = 0; i < plantLevel_; ++i) {
    const int baseX = 11 + ((i * 71 + i * i * 13) % 298);
    const int direction = (i & 1) ? -1 : 1;
    const int height = 10 + ((i * 7) % 14);
    const int localSway = sway + static_cast<int>(i % 3) - 1;
    canvas_.drawLine(baseX, TANK_BOTTOM, baseX + localSway, TANK_BOTTOM - height, plant);
    canvas_.drawLine(baseX + direction * 3, TANK_BOTTOM,
                     baseX + direction * 5 + localSway, TANK_BOTTOM - height + 5, plant);
    if (i % 2 == 0) {
      canvas_.drawLine(baseX - direction * 2, TANK_BOTTOM,
                       baseX - direction * 4 - localSway / 2, TANK_BOTTOM - height + 8, plant);
    }
  }
}

void AquariumEngine::drawCrab(unsigned long nowMs, uint16_t background) {
  // Permanent seabed resident. It patrols a small territory, pauses regularly
  // and waves a claw while stationary so it feels separate from the fish AI.
  const unsigned long cycle = nowMs % 24000UL;
  float progress;
  bool moving = true;
  if (cycle < 7000UL) progress = cycle / 7000.0f;
  else if (cycle < 10000UL) { progress = 1.0f; moving = false; }
  else if (cycle < 17000UL) progress = 1.0f - (cycle - 10000UL) / 7000.0f;
  else { progress = 0.0f; moving = false; }

  const int x = 205 + static_cast<int>(progress * 62.0f);
  const int y = TANK_BOTTOM - 4;
  const uint16_t crab = blend565(background, TFT_ORANGE, 220);
  const uint16_t dark = blend565(background, TFT_RED, 175);
  canvas_.fillRoundRect(x - 5, y - 4, 11, 6, 2, crab);
  canvas_.drawPixel(x - 2, y - 5, TFT_WHITE);
  canvas_.drawPixel(x + 3, y - 5, TFT_WHITE);
  canvas_.drawPixel(x - 2, y - 5, TFT_BLACK);
  canvas_.drawPixel(x + 3, y - 5, TFT_BLACK);
  for (int side = -1; side <= 1; side += 2) {
    canvas_.drawLine(x + side * 4, y, x + side * 8, y + 3, dark);
    canvas_.drawLine(x + side * 3, y - 1, x + side * 8, y - 3, crab);
  }
  const int clawLift = (!moving && ((nowMs / 450UL) & 1UL)) ? 3 : 0;
  canvas_.drawLine(x - 5, y - 3, x - 9, y - 6 - clawLift, crab);
  canvas_.drawCircle(x - 10, y - 7 - clawLift, 2, crab);
  canvas_.drawLine(x + 5, y - 3, x + 9, y - 6, crab);
  canvas_.drawCircle(x + 10, y - 7, 2, crab);
}

void AquariumEngine::drawWeatherEffects(
    unsigned long nowMs,
    uint16_t background,
    const OutdoorReading& outdoor) {
  if (!weatherEffectsEnabled_) return;
  (void)outdoor;

  if (ecologyRain_ > 0.08f) {
    const uint16_t rain = blend565(background, TFT_BLUE, 165);
    const int count = 3 + static_cast<int>(ecologyRain_ * 13.0f);
    const unsigned long fallSpeed = ecologyStorm_ > 0.3f ? 36UL : 55UL;
    for (int i = 0; i < count; ++i) {
      const int x = 12 + ((i * 37 + static_cast<int>(nowMs / 100UL)) % 296);
      const int y = 87 + ((i * 23 + static_cast<int>(nowMs / fallSpeed)) % 105);
      canvas_.drawPixel(x, y, rain);
      canvas_.drawPixel(x, y + 1, rain);
      if (ecologyRain_ > 0.72f) canvas_.drawPixel(x - 1, y + 2, rain);
    }
  }

  if (ecologySnow_ > 0.08f) {
    const uint16_t snow = blend565(background, TFT_WHITE, 180);
    const int count = 4 + static_cast<int>(ecologySnow_ * 12.0f);
    for (int i = 0; i < count; ++i) {
      const int drift = static_cast<int>(sinf((nowMs + i * 713UL) / 900.0f) * 5.0f);
      const int x = 12 + ((i * 29 + static_cast<int>(nowMs / 260UL) + drift) % 296);
      const int y = 87 + ((i * 19 + static_cast<int>(nowMs / 125UL)) % 106);
      canvas_.drawPixel(x, y, snow);
      if ((i & 3) == 0) canvas_.drawPixel(x + 1, y, snow);
    }
  }

  // Fog appears as very subtle horizontal veils rather than opaque blocks.
  if (ecologyFog_ > 0.10f) {
    const uint16_t haze = blend565(background, TFT_LIGHTGREY,
        static_cast<uint8_t>(35.0f + ecologyFog_ * 45.0f));
    const int drift = static_cast<int>((nowMs / 220UL) % 18UL);
    for (int y = 101; y < 190; y += 22) {
      for (int x = -30 + drift; x < 320; x += 54) {
        canvas_.drawFastHLine(x, y, 26, haze);
      }
    }
  }

  // Storm lightning is deliberately brief and irregular enough not to feel
  // like a repeating GIF. Intensity builds with the ecology transition.
  if (ecologyStorm_ > 0.20f) {
    const unsigned long cycle = (nowMs + 1730UL) % 12700UL;
    const unsigned long flashWindow = static_cast<unsigned long>(55.0f + ecologyStorm_ * 95.0f);
    if (cycle < flashWindow || (cycle > 210UL && cycle < 210UL + flashWindow / 2UL)) {
      const uint16_t flash = blend565(background, TFT_WHITE,
          static_cast<uint8_t>(65.0f + ecologyStorm_ * 100.0f));
      const int boltX = 276 + static_cast<int>((nowMs / 12700UL) % 4UL) * 7;
      canvas_.drawFastVLine(boltX, 87, 24, flash);
      canvas_.drawLine(boltX, 111, boltX - 6, 120, flash);
      canvas_.drawLine(boltX - 6, 120, boltX - 2, 128, flash);
    }
  }

  // At night a handful of dim blue specks give the water a quiet,
  // bioluminescent feel without compromising the dashboard text.
  if (ecologyDaylight_ < 0.28f) {
    const uint16_t glow = blend565(background, TFT_CYAN, 85);
    const int count = 2 + static_cast<int>((0.28f - ecologyDaylight_) * 12.0f);
    for (int i = 0; i < count; ++i) {
      const int x = 18 + ((i * 67 + static_cast<int>(nowMs / 800UL)) % 284);
      const int y = 101 + ((i * 31 + static_cast<int>(nowMs / 1200UL)) % 84);
      canvas_.drawPixel(x, y, glow);
    }
  }
}

void AquariumEngine::drawFishSprite(
    int x,
    int y,
    int8_t direction,
    uint8_t size,
    uint16_t colour,
    uint32_t phase,
    unsigned long nowMs,
    bool special) {
  const int d = direction > 0 ? 1 : -1;
  const int bodyW = 7 * size;
  const int bodyH = 4 * size;
  const int left = direction > 0 ? x : x - bodyW;
  canvas_.fillRoundRect(left, y - bodyH / 2, bodyW, bodyH, size, colour);

  // Tail flick alternates every few frames and differs per fish phase.
  const int tailSwing = (((nowMs + phase) / 180UL) % 2UL == 0UL) ? -1 : 1;
  const int tailX = direction > 0 ? left : left + bodyW;
  canvas_.drawLine(tailX, y,
                   tailX - d * 4 * size,
                   y - 3 * size + tailSwing * size,
                   colour);
  canvas_.drawLine(tailX, y,
                   tailX - d * 4 * size,
                   y + 3 * size + tailSwing * size,
                   colour);

  // Tiny dorsal fin makes the fish read more clearly at a glance.
  canvas_.drawLine(left + bodyW / 2, y - bodyH / 2,
                   left + bodyW / 2 - d * size, y - bodyH / 2 - 2 * size,
                   colour);

  const int eyeX = direction > 0 ? left + bodyW - 2 * size : left + size;
  canvas_.drawPixel(eyeX, y - size, TFT_BLACK);
  if (special) canvas_.drawPixel(left + bodyW / 2, y, TFT_WHITE);
}

void AquariumEngine::drawFishSchool(unsigned long nowMs, uint16_t background) {
  for (uint8_t i = 0; i < fishCount_; ++i) {
    uint16_t colour = blend565(background, fish_[i].colour, 185);
    drawFishSprite(static_cast<int>(fish_[i].x),
                   static_cast<int>(fish_[i].y),
                   fish_[i].direction,
                   fish_[i].size,
                   colour,
                   fish_[i].phase,
                   nowMs,
                   false);
  }
}

void AquariumEngine::drawFood(unsigned long nowMs, uint16_t background) {
  if (!feedingActive(nowMs)) return;
  const uint16_t food = blend565(background, TFT_ORANGE, 220);
  const unsigned long elapsed = nowMs - feedingStartedMs_;

  // Pellets appear in waves rather than as a permanently looping column.
  for (uint8_t i = 0; i < 10; ++i) {
    const unsigned long startDelay = i * 130UL;
    if (elapsed < startDelay) continue;
    const unsigned long pelletAge = elapsed - startDelay;
    if (pelletAge > 4700UL) continue;

    const int spread = static_cast<int>((i * 17 + (i % 3) * 7) % 49) - 24;
    const int x = constrain(feedingX_ + spread, TANK_LEFT + 3, TANK_RIGHT - 3);
    const int y = TANK_TOP + 4 + static_cast<int>(pelletAge / 42UL);
    if (y <= TANK_BOTTOM - 5) canvas_.fillCircle(x, y, 1, food);
  }
}

void AquariumEngine::drawSubmarine(
    int x, int y, int8_t direction, uint16_t colour) {
  const int d = direction > 0 ? 1 : -1;
  const int left = direction > 0 ? x : x - 27;
  canvas_.fillRoundRect(left, y - 6, 27, 12, 5, colour);
  canvas_.fillRect(left + 10, y - 10, 8, 5, colour);
  canvas_.drawFastVLine(left + 14, y - 14, 5, colour);
  canvas_.drawLine(left + 14, y - 14, left + 14 + 5 * d, y - 14, colour);
  canvas_.drawCircle(left + 7, y, 2, TFT_BLACK);
  canvas_.drawCircle(left + 20, y, 2, TFT_BLACK);
  const int propX = direction > 0 ? left - 3 : left + 30;
  canvas_.drawFastVLine(propX, y - 4, 9, colour);
}

void AquariumEngine::drawSquid(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const int d = direction > 0 ? 1 : -1;
  const int pulse = static_cast<int>((nowMs / 180UL) % 3UL);
  const int noseX = x + d * 9;
  const int rearX = x - d * 7;
  canvas_.fillTriangle(noseX, y, rearX, y - 7, rearX, y + 7, colour);
  canvas_.fillTriangle(x - d * 2, y - 4, rearX - d * 5, y - 9,
                       rearX, y - 2, colour);
  canvas_.fillTriangle(x - d * 2, y + 4, rearX - d * 5, y + 9,
                       rearX, y + 2, colour);
  canvas_.drawPixel(x + d * 4, y - 2, TFT_BLACK);
  for (int i = -4; i <= 4; i += 4) {
    canvas_.drawLine(rearX, y + i / 2,
                     rearX - d * (7 + pulse), y + i, colour);
  }
}

void AquariumEngine::drawOctopus(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const int sway = static_cast<int>((nowMs / 260UL) % 5UL) - 2;
  canvas_.fillCircle(x, y - 7, 7, colour);
  canvas_.fillRect(x - 7, y - 7, 15, 7, colour);
  canvas_.drawPixel(x + direction * 3, y - 9, TFT_BLACK);
  for (int i = -6; i <= 6; i += 3) {
    const int curl = ((i / 3) & 1) ? sway : -sway;
    canvas_.drawLine(x + i, y - 1, x + i + curl, y + 6, colour);
    canvas_.drawPixel(x + i + curl + direction, y + 7, colour);
  }
}

void AquariumEngine::drawPufferFish(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const bool puffed = ((nowMs - eventStartedMs_) / 1800UL) % 3UL == 1UL;
  const int radius = puffed ? 7 : 5;
  canvas_.fillCircle(x, y, radius, colour);
  const int d = direction > 0 ? 1 : -1;
  canvas_.fillTriangle(x - d * radius, y,
                       x - d * (radius + 5), y - 3,
                       x - d * (radius + 5), y + 3, colour);
  canvas_.drawPixel(x + d * 2, y - 2, TFT_BLACK);
  if (puffed) {
    for (int a = 0; a < 8; ++a) {
      const float angle = a * 0.785398f;
      const int sx = x + static_cast<int>(cosf(angle) * (radius + 1));
      const int sy = y + static_cast<int>(sinf(angle) * (radius + 1));
      const int ex = x + static_cast<int>(cosf(angle) * (radius + 3));
      const int ey = y + static_cast<int>(sinf(angle) * (radius + 3));
      canvas_.drawLine(sx, sy, ex, ey, colour);
    }
  }
}

void AquariumEngine::drawSeahorse(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const int d = direction > 0 ? 1 : -1;
  const int sway = static_cast<int>(sinf(nowMs / 320.0f) * 2.0f);
  canvas_.fillCircle(x, y - 6, 4, colour);
  canvas_.drawPixel(x + d * 2, y - 7, TFT_BLACK);
  canvas_.drawLine(x, y - 2, x - d * 2, y + 7, colour);
  canvas_.drawLine(x - d * 2, y + 7, x + sway, y + 11, colour);
  canvas_.drawCircle(x + sway + d * 2, y + 11, 2, colour);
  canvas_.drawLine(x - d * 1, y - 4, x - d * 5, y - 1, colour);
}

void AquariumEngine::drawWhale(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const int d = direction > 0 ? 1 : -1;
  const int left = direction > 0 ? x : x - 38;
  canvas_.fillRoundRect(left, y - 8, 38, 16, 7, colour);
  const int tailX = direction > 0 ? left : left + 38;
  canvas_.fillTriangle(tailX, y, tailX - d * 10, y - 8,
                       tailX - d * 6, y, colour);
  canvas_.fillTriangle(tailX, y, tailX - d * 10, y + 8,
                       tailX - d * 6, y, colour);
  const int finX = direction > 0 ? left + 20 : left + 18;
  canvas_.fillTriangle(finX, y + 5, finX - d * 6, y + 12,
                       finX + d * 3, y + 6, colour);
  const int eyeX = direction > 0 ? left + 31 : left + 6;
  canvas_.drawPixel(eyeX, y - 3, TFT_BLACK);
  const int spout = static_cast<int>((nowMs - eventStartedMs_) % 6500UL);
  if (spout < 700) {
    canvas_.drawLine(eyeX - d * 6, y - 8, eyeX - d * 6, y - 15, TFT_CYAN);
    canvas_.drawLine(eyeX - d * 6, y - 15, eyeX - d * 10, y - 18, TFT_CYAN);
    canvas_.drawLine(eyeX - d * 6, y - 15, eyeX - d * 2, y - 18, TFT_CYAN);
  }
}

void AquariumEngine::drawRareVisitor(
    int x, int y, int8_t direction, RareSpecies species,
    uint16_t colour, unsigned long nowMs) {
  switch (species) {
    case RareSpecies::Koi:
      drawFishSprite(x, y, direction, 2, colour, 0x1267u, nowMs, true);
      break;
    case RareSpecies::Jellyfish:
      canvas_.fillCircle(x, y - 2, 6, colour);
      canvas_.fillRect(x - 6, y - 2, 13, 4, colour);
      for (int8_t i = -4; i <= 4; i += 4) {
        canvas_.drawFastVLine(x + i, y + 2, 7 + ((i + 4) / 2), colour);
      }
      break;
    case RareSpecies::AnglerFish:
      drawFishSprite(x, y, direction, 2, colour, 0x8855u, nowMs, true);
      canvas_.drawLine(x, y - 5, x + direction * 6, y - 10, colour);
      canvas_.fillCircle(x + direction * 7, y - 10, 1, TFT_WHITE);
      break;
    case RareSpecies::Shark:
      drawFishSprite(x, y, direction, 2, colour, 0x4411u, nowMs, false);
      canvas_.fillTriangle(x - direction * 2, y - 4,
                           x - direction * 7, y - 10,
                           x + direction * 3, y - 5, colour);
      break;
    case RareSpecies::Squid:
      drawSquid(x, y, direction, colour, nowMs);
      break;
    case RareSpecies::Octopus:
      drawOctopus(x, y, direction, colour, nowMs);
      break;
    case RareSpecies::PufferFish:
      drawPufferFish(x, y, direction, colour, nowMs);
      break;
    case RareSpecies::Seahorse:
      drawSeahorse(x, y, direction, colour, nowMs);
      break;
    case RareSpecies::Whale:
      drawWhale(x, y, direction, colour, nowMs);
      break;
    default:
      break;
  }
}

void AquariumEngine::drawSpecialEvent(unsigned long nowMs, uint16_t background) {
  if (eventType_ == EventType::None ||
      static_cast<long>(eventUntilMs_ - nowMs) <= 0) return;

  const int y = static_cast<int>(eventY_);
  if (eventType_ == EventType::Submarine) {
    drawSubmarine(static_cast<int>(eventX_), y, eventDirection_,
                  blend565(background, TFT_YELLOW, 220));
    return;
  }

  uint16_t baseColour = TFT_ORANGE;
  if (eventType_ == EventType::NocturnalVisitor) baseColour = TFT_MAGENTA;
  if (eventType_ == EventType::LegendaryVisitor) baseColour = TFT_CYAN;
  if (rareSpecies_ == RareSpecies::Squid ||
      rareSpecies_ == RareSpecies::Jellyfish) baseColour = TFT_CYAN;
  if (rareSpecies_ == RareSpecies::Seahorse) baseColour = TFT_GREEN;
  if (rareSpecies_ == RareSpecies::PufferFish) baseColour = TFT_YELLOW;

  const uint16_t colour = blend565(background, baseColour,
      eventType_ == EventType::LegendaryVisitor ? 235 : 220);
  drawRareVisitor(static_cast<int>(eventX_), y, eventDirection_,
                  rareSpecies_, colour, nowMs);
}
