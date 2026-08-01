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

int parseClockSeconds(const String& text) {
  if (text.length() < 8) return 0;
  const int lastColon = text.lastIndexOf(':');
  if (lastColon < 0 || lastColon + 2 >= static_cast<int>(text.length())) return 0;
  return constrain(text.substring(lastColon + 1, lastColon + 3).toInt(), 0, 59);
}

bool dateIs(const String& dateText, const char* suffix) {
  return dateText.endsWith(suffix);
}

bool isFriday13(const String& dateText) {
  return dateText.startsWith("Friday 13 ");
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
  float windDirection = ecologyWindDirectionDeg_;

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
    if (!isnan(outdoor.windDirectionDeg)) {
      windDirection = outdoor.windDirectionDeg;
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
    ecologyWindDirectionDeg_ = windDirection;
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
  // Direction changes are only used for gentle visual bias, so a simple
  // smoothed angle is sufficient even when crossing north/360 degrees.
  ecologyWindDirectionDeg_ = approachFloat(ecologyWindDirectionDeg_, windDirection, weatherAlpha);
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

  if (species == RareSpecies::Whale) return rarity == EventRarity::Legendary;
  if (species == RareSpecies::SwampMonster) return rarity == EventRarity::Legendary && night;
  if (rarity == EventRarity::Legendary) return false;

  // Nocturnal creatures are possible by day, but are strongly favoured at night.
  if ((species == RareSpecies::AnglerFish || species == RareSpecies::Squid) &&
      !night && rarity == EventRarity::Uncommon) return false;

  // Seahorses favour calmer conditions. Sharks are less likely in storms/snow.
  if (outdoor.valid) {
    if (species == RareSpecies::Seahorse &&
        (isStormCode(outdoor.weatherCode) || isSnowCode(outdoor.weatherCode)))
      return false;
    if ((species == RareSpecies::Shark || species == RareSpecies::Hammerhead) &&
        isSnowCode(outdoor.weatherCode))
      return false;
  }

  return true;
}

AquariumEngine::RareSpecies AquariumEngine::chooseRareSpecies(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    EventRarity rarity) {
  if (rarity == EventRarity::Legendary) {
    // The retro swamp monster is a night-only legendary visitor. During the
    // day the legendary slot remains the whale.
    if (isNight(clock) && (nextRandom() % 100u) < 45u) return RareSpecies::SwampMonster;
    return RareSpecies::Whale;
  }

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
    // Hammerheads are a rarer sight than the normal shark.
    if (candidate == RareSpecies::Hammerhead && rarity == EventRarity::Uncommon) continue;
    if (candidate == RareSpecies::Hammerhead && rarity == EventRarity::Rare &&
        (nextRandom() % 2u) != 0u) continue;
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
  if (type == EventType::LegendaryVisitor) return species == RareSpecies::SwampMonster ? 42000UL : 30000UL;
  switch (species) {
    case RareSpecies::Octopus: return 26000UL;
    case RareSpecies::Jellyfish: return 23000UL;
    case RareSpecies::Seahorse: return 24000UL;
    case RareSpecies::Squid: return 17000UL;
    case RareSpecies::Shark: return 15000UL;
    case RareSpecies::Hammerhead: return 17500UL;
    default: return 19000UL;
  }
}

float AquariumEngine::eventBaseSpeed(RareSpecies species, EventType type) const {
  if (type == EventType::Submarine) return 18.0f;
  if (type == EventType::LegendaryVisitor) return species == RareSpecies::SwampMonster ? 9.5f : 11.0f;
  switch (species) {
    case RareSpecies::Octopus: return 6.5f;
    case RareSpecies::Jellyfish: return 11.0f;
    case RareSpecies::Seahorse: return 9.0f;
    case RareSpecies::PufferFish: return 15.0f;
    case RareSpecies::Squid: return 24.0f;
    case RareSpecies::Shark: return 31.0f;
    case RareSpecies::Hammerhead: return 27.0f;
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
    rareSpecies_ = (night && (nextRandom() % 100u) < 45u)
                       ? RareSpecies::SwampMonster : RareSpecies::Whale;
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
  if (rareSpecies_ == RareSpecies::Shark) eventY_ = 146.0f;
  if (rareSpecies_ == RareSpecies::Hammerhead) eventY_ = 150.0f;
  if (rareSpecies_ == RareSpecies::Whale) eventY_ = 150.0f;
  if (rareSpecies_ == RareSpecies::SwampMonster) eventY_ = 171.0f;

  eventStartedMs_ = nowMs;
  eventUntilMs_ = nowMs + eventDurationFor(rareSpecies_, eventType_);

  if (eventType_ != EventType::Submarine) {
    previousSpecies_ = lastSpecies_;
    lastSpecies_ = rareSpecies_;
  }
  lastEventType_ = eventType_;
  scheduleNextEvent(eventUntilMs_);
}

const char* AquariumEngine::demoSceneLabel() const {
  static const char* labels[] = {
    "RESIDENT AQUARIUM", "FEEDING", "CLEAR DAY", "DAWN", "SUNSET", "NIGHT + STARS",
    "CLOUD", "FOG", "RAIN", "STORM", "SNOW", "HIGH WIND",
    "KOI", "JELLYFISH", "ANGLER FISH", "SHARK", "HAMMERHEAD", "SQUID",
    "OCTOPUS", "PUFFER FISH", "SEAHORSE", "WHALE", "SWAMP MONSTER", "SUBMARINE",
    "GHOST HOUR", "FRIDAY 13TH", "HALLOWEEN", "CHRISTMAS", "VALENTINE",
    "APRIL FOOL", "LEAP FISH", "NEW YEAR", "DAWN / DUSK RAY"
  };
  return demoScene_ < (sizeof(labels) / sizeof(labels[0])) ? labels[demoScene_] : "LIVE";
}

void AquariumEngine::clearDemo(unsigned long nowMs) {
  demoScene_ = 255;
  eventType_ = EventType::None;
  calendarLabel_ = "";
  feedingUntilMs_ = 0;
  ecologySeeded_ = false;
  scheduleNextEvent(nowMs);
}

void AquariumEngine::setDemoScene(uint8_t scene, unsigned long nowMs) {
  demoScene_ = scene;
  eventType_ = EventType::None;
  eventUntilMs_ = 0;
  calendarLabel_ = "";
  feedingUntilMs_ = 0;
  ecologySeeded_ = false;

  if (scene == 1) feed(nowMs, 160, 105);
  if (scene < 12 || scene > 23) return;

  eventType_ = EventType::Visitor;
  eventRarity_ = EventRarity::Rare;
  if (scene == 23) {
    eventType_ = EventType::Submarine;
    rareSpecies_ = RareSpecies::Koi;
  } else {
    rareSpecies_ = static_cast<RareSpecies>(scene - 12);
    if (rareSpecies_ == RareSpecies::Whale || rareSpecies_ == RareSpecies::SwampMonster) {
      eventType_ = EventType::LegendaryVisitor;
      eventRarity_ = EventRarity::Legendary;
    } else if (rareSpecies_ == RareSpecies::AnglerFish || rareSpecies_ == RareSpecies::Squid ||
               rareSpecies_ == RareSpecies::Jellyfish) {
      eventType_ = EventType::NocturnalVisitor;
    }
  }

  eventDirection_ = 1;
  eventX_ = -48.0f;
  eventY_ = 137.0f;
  if (eventType_ == EventType::Submarine) eventY_ = 183.0f;
  if (rareSpecies_ == RareSpecies::Octopus) eventY_ = 190.0f;
  if (rareSpecies_ == RareSpecies::Squid) eventY_ = 135.0f;
  if (rareSpecies_ == RareSpecies::Jellyfish) eventY_ = 128.0f;
  if (rareSpecies_ == RareSpecies::Seahorse) eventY_ = 155.0f;
  if (rareSpecies_ == RareSpecies::PufferFish) eventY_ = 145.0f;
  if (rareSpecies_ == RareSpecies::Shark) eventY_ = 146.0f;
  if (rareSpecies_ == RareSpecies::Hammerhead) eventY_ = 150.0f;
  if (rareSpecies_ == RareSpecies::Whale) eventY_ = 150.0f;
  if (rareSpecies_ == RareSpecies::SwampMonster) eventY_ = 171.0f;
  eventVelocity_ = 0.0f;
  eventStartedMs_ = nowMs;
  eventNextActionMs_ = nowMs + 1200UL;
  eventUntilMs_ = nowMs + 600000UL; // stays present until the showcase advances
}

void AquariumEngine::configure(uint8_t fishCount, uint8_t bubbleLevel, uint8_t plantLevel,
                               uint8_t plantLength, uint8_t animationLevel,
                               bool weatherEffects, uint8_t eventFrequency, bool jellyfishEnabled) {
  fishCount_ = constrain((int)fishCount, 1, 10);
  bubbleLevel_ = constrain((int)bubbleLevel, 0, 10);
  plantLevel_ = constrain((int)plantLevel, 0, 10);
  plantLength_ = constrain((int)plantLength, 1, 15);
  animationLevel_ = constrain((int)animationLevel, 0, 3);
  eventFrequency_ = constrain((int)eventFrequency, 0, 3);
  jellyfishEnabled_ = jellyfishEnabled;
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
  if (calendarLabel_.length()) return calendarLabel_.c_str();
  if (static_cast<long>(eventUntilMs_ - nowMs) <= 0) return "";
  switch (eventType_) {
    case EventType::Visitor: return "SPECIAL VISITOR";
    case EventType::Submarine: return "SUBMARINE";
    case EventType::NocturnalVisitor: return "NIGHT VISITOR";
    case EventType::LegendaryVisitor: return rareSpecies_ == RareSpecies::SwampMonster ? "SOMETHING IN THE WATER..." : "LEGENDARY VISITOR";
    default: return "";
  }
}

void AquariumEngine::update(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  if (!initialised_) begin(nowMs);
  updateEcology(nowMs, clock, outdoor);
  updateCalendarEvent(clock, outdoor);
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

    // v0.1.8 schooling: nearby residents gently avoid overlapping while
    // social fish still bias toward the school. The effect is deliberately
    // subtle so the dashboard remains calm rather than looking chaotic.
    if (!feeding && fish.state == FishState::Cruise) {
      float separationY = 0.0f;
      float neighbourSpeed = 0.0f;
      uint8_t neighbours = 0;
      for (uint8_t j = 0; j < fishCount_; ++j) {
        if (j == i) continue;
        const float dxn = fish_[j].x - fish.x;
        const float dyn = fish_[j].y - fish.y;
        if (fabsf(dxn) < 34.0f && fabsf(dyn) < 22.0f) {
          ++neighbours;
          neighbourSpeed += fabsf(fish_[j].vx);
          if (fabsf(dyn) < 10.0f) separationY += (dyn >= 0.0f) ? -1.0f : 1.0f;
        }
      }
      if (neighbours > 0) {
        targetY += separationY * 3.4f;
        if (fish.personality == Personality::Social) {
          const float avgSpeed = neighbourSpeed / neighbours;
          if (avgSpeed > 1.0f) speedFactor *= 0.92f + min(avgSpeed / 80.0f, 0.16f);
        }
      }
    }

    // Sharks change the behaviour of the resident school. Fish close to the
    // visitor peel away vertically and accelerate for a moment rather than
    // ignoring a large predator swimming straight through the aquarium.
    const bool predatorActive = eventType_ != EventType::None &&
        (rareSpecies_ == RareSpecies::Shark || rareSpecies_ == RareSpecies::Hammerhead) &&
        static_cast<long>(eventUntilMs_ - nowMs) > 0;
    if (!feeding && predatorActive) {
      const float predatorDx = eventX_ - fish.x;
      const float predatorDy = eventY_ - fish.y;
      if (fabsf(predatorDx) < 74.0f && fabsf(predatorDy) < 42.0f) {
        targetY += (predatorDy >= 0.0f) ? -20.0f : 20.0f;
        targetY = clampFloat(targetY, 96.0f, 190.0f);
        speedFactor *= 1.55f;
        if (fabsf(predatorDx) < 42.0f) {
          fish.direction = predatorDx > 0.0f ? -1 : 1;
        }
      }
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
  if (demoScene_ == 255 && eventType_ == EventType::None &&
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
  drawSkyAtmosphere(clock, outdoor, nowMs, background);
  drawCelestial(clock, outdoor, nowMs, background);
  drawWaterSurface(nowMs, background, outdoor);
  drawHourlyBoat(clock, nowMs, background);
  drawWeatherEffects(nowMs, background, outdoor);
  drawBubbles(nowMs, background, clock, outdoor);
  drawPlants(nowMs, background);
  drawCrab(nowMs, background);
  drawJellyfish(nowMs, background);
  drawFood(nowMs, background);
  drawFishSchool(nowMs, background);
  drawSpecialEvent(nowMs, background);
  drawCalendarEvents(clock, outdoor, nowMs, background);
}

void AquariumEngine::drawSkyAtmosphere(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    unsigned long nowMs,
    uint16_t background) {
  if (!clock.valid) return;
  const int nowMinutes = parseClockMinutes(clock.timeText);
  int sunrise = outdoor.valid ? parseClockMinutes(outdoor.sunriseText) : 6 * 60;
  int sunset = outdoor.valid ? parseClockMinutes(outdoor.sunsetText) : 21 * 60;
  if (nowMinutes < 0) return;
  if (sunrise < 0 || sunset <= sunrise) { sunrise = 6 * 60; sunset = 21 * 60; }

  // A restrained three-band sky tint gives dawn/day/dusk/night a sense of
  // progression while remaining behind the dashboard information.
  float daylight = daylightFactor(clock, outdoor);
  uint16_t horizon = background;
  if (daylight > 0.72f) horizon = blend565(background, TFT_CYAN, 28);
  else if (daylight > 0.30f) horizon = blend565(background, TFT_ORANGE, 45);
  else horizon = blend565(background, TFT_NAVY, 90);
  canvas_.fillRect(0, 30, 320, 18, blend565(background, horizon, 105));
  canvas_.fillRect(0, 48, 320, 18, blend565(background, horizon, 70));
  canvas_.fillRect(0, 66, 320, 16, blend565(background, horizon, 42));

  // Stars fade in after sunset, peak through the middle of the night, and
  // progressively disappear before sunrise. Positions are deterministic so
  // the sky feels stable rather than like random snow.
  if (daylight < 0.38f) {
    const float darkness = clampFloat((0.38f - daylight) / 0.38f, 0.0f, 1.0f);
    const int starCount = 3 + static_cast<int>(darkness * 17.0f);
    const uint16_t dim = blend565(background, TFT_WHITE,
                                  static_cast<uint8_t>(55 + darkness * 110));
    for (int i = 0; i < starCount; ++i) {
      const int x = 9 + ((i * 73 + i * i * 19) % 302);
      const int y = 33 + ((i * 29 + i * i * 7) % 46);
      const bool twinkle = ((nowMs / (700UL + (i % 4) * 170UL) + i) & 1UL) != 0;
      canvas_.drawPixel(x, y, twinkle ? TFT_WHITE : dim);
      if (darkness > 0.82f && (i % 6) == 0) canvas_.drawPixel(x + 1, y, dim);
    }
  }


  // Visible layered clouds, scaled by live cloud cover.
  const int cloudCount = 1 + static_cast<int>(ecologyCloud_ * 5.0f);
  for (int i = 0; i < cloudCount; ++i) {
    int x = ((static_cast<int>(nowMs / (180UL + i * 35UL)) + i * 79) % 390) - 40;
    int y = 36 + (i * 11) % 34;
    uint16_t cc = blend565(background, ecologyStorm_ > 0.35f ? TFT_DARKGREY : TFT_WHITE, 75 + i * 12);
    canvas_.fillCircle(x, y, 5 + (i % 3), cc);
    canvas_.fillCircle(x + 8, y - 2, 7 + (i % 2), cc);
    canvas_.fillCircle(x + 16, y, 5 + (i % 3), cc);
    canvas_.fillRect(x, y, 17, 6, cc);
  }
  // Occasional bird flock; deterministic 18-minute cycle.
  unsigned long birdPhase = nowMs % 1080000UL;
  if (birdPhase < 28000UL && ecologyDaylight_ > 0.35f) {
    int bx = -20 + static_cast<int>(birdPhase * 360UL / 28000UL);
    uint16_t bc = blend565(background, TFT_BLACK, 170);
    for (int i=0;i<4;++i) { int x=bx-i*9, y=43+abs(2-i)*4; canvas_.drawLine(x-2,y,x,y-2,bc); canvas_.drawLine(x,y-2,x+2,y,bc); }
  }
  // Wind-blown autumnal leaves when wind is noticeable.
  if (ecologyWind_ > 0.45f) {
    for (int i=0;i<3;++i) { int x=(static_cast<int>(nowMs/(38+i*9))+i*103)%330-5; int y=42+((static_cast<int>(nowMs/(91+i*13))+i*17)%35); uint16_t lc=(i&1)?TFT_ORANGE:TFT_YELLOW; canvas_.drawPixel(x,y,lc); canvas_.drawPixel(x+1,y+((nowMs/180+i)&1),lc); }
  }
  // Tiny warm-weather dragonfly; rare bats replace it after dusk.
  unsigned long lifePhase = nowMs % 1320000UL;
  if (lifePhase < 18000UL && ecologyDaylight_ > 0.65f && ecologyRain_ < 0.15f) {
    int dx = -5 + static_cast<int>(lifePhase * 330UL / 18000UL);
    int dy = 58 + static_cast<int>(sinf(lifePhase / 280.0f) * 5.0f);
    canvas_.drawPixel(dx, dy, TFT_CYAN); canvas_.drawPixel(dx-1,dy-1,TFT_WHITE); canvas_.drawPixel(dx+1,dy+1,TFT_WHITE);
  } else if (lifePhase < 16000UL && ecologyDaylight_ < 0.22f) {
    int bx = 325 - static_cast<int>(lifePhase * 345UL / 16000UL);
    uint16_t bat = blend565(background, TFT_DARKGREY, 190);
    canvas_.drawLine(bx-3,53,bx,55,bat); canvas_.drawLine(bx,55,bx+3,53,bat);
  }
}

void AquariumEngine::drawCelestial(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    unsigned long nowMs,
    uint16_t background) {
  (void)nowMs;
  if (!clock.valid) return;

  int nowMinutes = parseClockMinutes(clock.timeText);
  int sunrise = outdoor.valid ? parseClockMinutes(outdoor.sunriseText) : -1;
  int sunset = outdoor.valid ? parseClockMinutes(outdoor.sunsetText) : -1;
  if (sunrise < 0 || sunset < 0 || sunset <= sunrise) {
    sunrise = 6 * 60;
    sunset = 21 * 60;
  }
  if (nowMinutes < 0) return;

  // The celestial body lives behind the dashboard text. It is intentionally
  // small and subdued: useful atmosphere, never a replacement for the clock.
  constexpr int LEFT_X = 24;
  constexpr int RIGHT_X = 296;
  constexpr int HORIZON_Y = 75;
  constexpr int ARC_HEIGHT = 42;

  bool sun = nowMinutes >= sunrise && nowMinutes <= sunset;
  float progress = 0.0f;
  if (sun) {
    progress = static_cast<float>(nowMinutes - sunrise) /
               static_cast<float>(sunset - sunrise);
  } else {
    const int nightLength = (24 * 60 - sunset) + sunrise;
    int elapsed = nowMinutes >= sunset ? nowMinutes - sunset
                                      : (24 * 60 - sunset) + nowMinutes;
    progress = nightLength > 0 ? static_cast<float>(elapsed) /
                                 static_cast<float>(nightLength) : 0.5f;
  }
  progress = clampFloat(progress, 0.0f, 1.0f);
  const float arc = sinf(progress * 3.14159265f);
  const int x = LEFT_X + static_cast<int>((RIGHT_X - LEFT_X) * progress);
  const int y = HORIZON_Y - static_cast<int>(ARC_HEIGHT * arc);

  if (sun) {
    const uint16_t glow = blend565(background, TFT_YELLOW, 135);
    const uint16_t core = blend565(background, TFT_ORANGE, 205);
    canvas_.drawCircle(x, y, 7, glow);
    canvas_.fillCircle(x, y, 5, core);
    // Four tiny rays are enough to read as a sun without clutter.
    canvas_.drawPixel(x - 9, y, glow);
    canvas_.drawPixel(x + 9, y, glow);
    canvas_.drawPixel(x, y - 9, glow);
    canvas_.drawPixel(x, y + 9, glow);
  } else {
    const uint16_t moon = blend565(background, TFT_WHITE, 165);
    canvas_.fillCircle(x, y, 6, moon);
    // Paint a background-coloured offset circle over it to form a crescent.
    canvas_.fillCircle(x + 3, y - 2, 5, background);
    canvas_.drawPixel(x - 4, y - 1, blend565(background, TFT_LIGHTGREY, 110));
  }
}

void AquariumEngine::drawWaterSurface(
    unsigned long nowMs,
    uint16_t background,
    const OutdoorReading& outdoor) {
  (void)outdoor;
  const uint16_t water = blend565(background, TFT_CYAN,
      static_cast<uint8_t>(80.0f + ecologySurface_ * 95.0f));
  const int speed = static_cast<int>(220.0f - ecologySurface_ * 145.0f);
  const float windRadians = ecologyWindDirectionDeg_ * 0.0174532925f;
  const int windSign = cosf(windRadians) >= 0.0f ? 1 : -1;
  const int shift = windSign * static_cast<int>((nowMs / max(speed, 60)) % 8UL);
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
  const uint16_t microBubble = blend565(background, TFT_WHITE, 105);
  (void)clock;
  (void)outdoor;
  float rate = ecologyBubbleRate_;
  if (rate < 0.45f) rate = 0.45f;

  // v0.1.8: bubbles rise in loosely grouped streams with a gentle sine drift,
  // plus occasional tiny companion bubbles. This reads more like an aerated
  // aquarium than uniformly distributed moving dots.
  const unsigned long period = static_cast<unsigned long>(6200.0f / rate);
  const uint8_t bubbleCount = bubbleLevel_ * 2;
  for (uint8_t i = 0; i < bubbleCount; ++i) {
    const int stream = i / 2;
    const int baseX = TANK_LEFT + 9 + ((stream * 67 + stream * stream * 13) %
                                      (TANK_RIGHT - TANK_LEFT - 18));
    const uint32_t offset = static_cast<uint32_t>(i) * 733UL +
                            static_cast<uint32_t>(stream % 5) * 1271UL;
    const unsigned long phase = (nowMs + offset) % period;
    const int y = TANK_BOTTOM - static_cast<int>((phase * (TANK_BOTTOM - 88)) / period);
    const float wave = sinf((nowMs + offset) / (430.0f + (stream % 3) * 90.0f));
    const float windRadians = ecologyWindDirectionDeg_ * 0.0174532925f;
    const float windBias = cosf(windRadians) * ecologyWind_ * 4.0f;
    const int drift = static_cast<int>(wave * (2.0f + ecologySurface_ * 2.0f) + windBias);
    const int radius = (i % 5 == 0) ? 2 : 1;
    canvas_.drawCircle(baseX + drift, y, radius, bubble);

    if ((i % 3) == 0 && y < TANK_BOTTOM - 8) {
      canvas_.drawPixel(baseX + drift + 3, y + 7, microBubble);
    }
  }
}

void AquariumEngine::drawPlants(unsigned long nowMs, uint16_t background) {
  if (plantLevel_ == 0) return;
  const uint16_t plant = blend565(background, TFT_GREEN, 155);
  const int swayRange = 2 + static_cast<int>(ecologySurface_ * 2.5f);
  const int phase = static_cast<int>((nowMs / 420UL) % static_cast<unsigned long>(swayRange * 2 + 1));
  const int sway = phase - swayRange;
  const float windRadians = ecologyWindDirectionDeg_ * 0.0174532925f;
  const int windLean = static_cast<int>(cosf(windRadians) * ecologyWind_ * 6.0f);

  // 0..10 maps directly to the number of plant clusters. Clusters alternate
  // across the seabed and vary in height so high density still looks organic.
  for (uint8_t i = 0; i < plantLevel_; ++i) {
    const int baseX = 11 + ((i * 71 + i * i * 13) % 298);
    const int direction = (i & 1) ? -1 : 1;
    const int naturalHeight = 10 + ((i * 7) % 14);
    // v0.1.10 extends plant length to 1..15. Around 7 retains the familiar
    // medium-height look, while 15 can produce tall, dramatic fronds.
    const float lengthScale = 0.55f + static_cast<float>(plantLength_) * 0.24f;
    const int variedNatural = naturalHeight + ((i * 11) % 13);
    const int height = constrain(static_cast<int>(variedNatural * lengthScale), 8, 86);
    const int localSway = sway + windLean + static_cast<int>(i % 3) - 1;
    canvas_.drawLine(baseX, TANK_BOTTOM, baseX + localSway, TANK_BOTTOM - height, plant);
    canvas_.drawLine(baseX + direction * 3, TANK_BOTTOM,
                     baseX + direction * 5 + localSway,
                     TANK_BOTTOM - max(4, height - 5), plant);
    if (i % 2 == 0) {
      canvas_.drawLine(baseX - direction * 2, TANK_BOTTOM,
                       baseX - direction * 4 - localSway / 2,
                       TANK_BOTTOM - max(3, height - 8), plant);
    }
  }
}

void AquariumEngine::drawCrab(unsigned long nowMs, uint16_t background) {
  // Tiny whimsical crab home tucked into the lower-right corner. It is kept
  // deliberately small (about 25 px wide) so it reads as background scenery
  // rather than competing with the dashboard information.
  constexpr int HOME_X = 287;
  constexpr int HOME_Y = 189;
  constexpr int HOME_W = 26;
  constexpr int HOME_H = 12;
  const uint16_t shell = blend565(background, TFT_ORANGE, 105);
  const uint16_t shellEdge = blend565(background, TFT_WHITE, 75);
  const uint16_t doorway = blend565(background, TFT_BLACK, 225);

  canvas_.fillRoundRect(HOME_X, HOME_Y, HOME_W, HOME_H, 6, shell);
  canvas_.drawRoundRect(HOME_X, HOME_Y, HOME_W, HOME_H, 6, shellEdge);
  canvas_.fillCircle(HOME_X + 7, TANK_BOTTOM - 3, 4, doorway);
  canvas_.fillRect(HOME_X + 3, TANK_BOTTOM - 3, 9, 4, doorway);
  canvas_.drawPixel(HOME_X + 18, HOME_Y + 3, shellEdge);
  canvas_.drawPixel(HOME_X + 21, HOME_Y + 6, shellEdge);

  // A small anemone/weed colony on the cave roof helps the crab home blend
  // into the seabed. It shares the same environmental sway as the main plants.
  const uint16_t hutPlant = blend565(background, TFT_GREEN, 170);
  const float hutWindRad = ecologyWindDirectionDeg_ * 0.0174532925f;
  const int hutLean = static_cast<int>(cosf(hutWindRad) * ecologyWind_ * 3.0f);
  const int hutSway = static_cast<int>(sinf(nowMs / 620.0f) * 2.0f) + hutLean;
  const int rootX = HOME_X + 18;
  canvas_.drawLine(rootX, HOME_Y + 1, rootX + hutSway, HOME_Y - 8, hutPlant);
  canvas_.drawLine(rootX - 2, HOME_Y + 1, rootX - 4 + hutSway, HOME_Y - 5, hutPlant);
  canvas_.drawLine(rootX + 2, HOME_Y + 1, rootX + 5 + hutSway, HOME_Y - 6, hutPlant);

  // Home -> emerge -> explore -> wave -> return -> home. The crab spends a
  // meaningful part of each cycle inside its cave, making its appearances
  // feel less like a repeating conveyor-belt animation.
  const unsigned long cycleNumber = nowMs / 52000UL;
  const unsigned long cycle = nowMs % 52000UL;
  const float exploreX = 216.0f + static_cast<float>((cycleNumber * 17UL) % 25UL);
  float crabX = 300.0f;
  bool visible = true;
  bool moving = true;
  bool waving = false;

  if (cycle < 9000UL) {
    visible = false;
    // A brief, slightly irregular peek prevents the crab feeling clockwork.
    const unsigned long peekStart = 3600UL + (cycleNumber % 4UL) * 420UL;
    if (cycle >= peekStart && cycle < peekStart + 1450UL) {
      canvas_.drawPixel(HOME_X + 5, TANK_BOTTOM - 5, TFT_WHITE);
      canvas_.drawPixel(HOME_X + 8, TANK_BOTTOM - 5, TFT_WHITE);
    }
  } else if (cycle < 13000UL) {
    const float p = (cycle - 9000UL) / 4000.0f;
    crabX = 300.0f - p * 20.0f;
  } else if (cycle < 30000UL) {
    const float p = (cycle - 13000UL) / 17000.0f;
    crabX = 280.0f + (exploreX - 280.0f) * p;
  } else if (cycle < 35500UL) {
    crabX = exploreX;
    moving = false;
    waving = true;
  } else if (cycle < 47500UL) {
    const float p = (cycle - 35500UL) / 12000.0f;
    crabX = exploreX + (300.0f - exploreX) * p;
  } else {
    visible = false;
  }

  if (!visible) return;

  const int x = static_cast<int>(crabX);
  const int y = TANK_BOTTOM - 4;
  const uint16_t crab = blend565(background, TFT_ORANGE, 225);
  const uint16_t dark = blend565(background, TFT_RED, 180);
  canvas_.fillRoundRect(x - 5, y - 4, 11, 6, 2, crab);

  // Eye stalks.
  canvas_.drawLine(x - 2, y - 4, x - 2, y - 7, crab);
  canvas_.drawLine(x + 3, y - 4, x + 3, y - 7, crab);
  canvas_.drawPixel(x - 2, y - 7, TFT_WHITE);
  canvas_.drawPixel(x + 3, y - 7, TFT_WHITE);

  // Legs wiggle slightly while walking.
  const int step = moving && ((nowMs / 180UL) & 1UL) ? 1 : 0;
  for (int side = -1; side <= 1; side += 2) {
    canvas_.drawLine(x + side * 4, y,
                     x + side * 8, y + 2 + step, dark);
    canvas_.drawLine(x + side * 3, y - 1,
                     x + side * 8, y - 3 - step, crab);
  }

  const int clawLift = (waving && ((nowMs / 360UL) & 1UL)) ? 4 : 0;
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

  // Precipitation belongs to the atmosphere, not inside the aquarium.
  // Keep every falling particle above the waterline at y=82; impacts are
  // represented separately by the small splashes in drawWaterSurface().
  constexpr int SKY_FX_TOP = 29;
  constexpr int SKY_FX_BOTTOM = 77;
  constexpr int SKY_FX_HEIGHT = SKY_FX_BOTTOM - SKY_FX_TOP + 1;

  if (ecologyRain_ > 0.08f) {
    const uint16_t rain = blend565(background, TFT_BLUE, 165);
    const int count = 3 + static_cast<int>(ecologyRain_ * 13.0f);
    const unsigned long fallSpeed = ecologyStorm_ > 0.3f ? 36UL : 55UL;
    for (int i = 0; i < count; ++i) {
      const int x = 12 + ((i * 37 + static_cast<int>(nowMs / 100UL)) % 296);
      const int y = SKY_FX_TOP +
          ((i * 23 + static_cast<int>(nowMs / fallSpeed)) % SKY_FX_HEIGHT);
      const float windRadians = ecologyWindDirectionDeg_ * 0.0174532925f;
      const int slant = static_cast<int>(cosf(windRadians) * ecologyWind_ * 4.0f);
      canvas_.drawPixel(x, y, rain);
      if (y + 1 <= SKY_FX_BOTTOM)
        canvas_.drawPixel(x + slant / 2, y + 1, rain);
      if (ecologyRain_ > 0.72f && y + 2 <= SKY_FX_BOTTOM)
        canvas_.drawPixel(x + slant, y + 2, rain);
    }
  }

  if (ecologySnow_ > 0.08f) {
    const uint16_t snow = blend565(background, TFT_WHITE, 180);
    const int count = 4 + static_cast<int>(ecologySnow_ * 12.0f);
    for (int i = 0; i < count; ++i) {
      const float windRadians = ecologyWindDirectionDeg_ * 0.0174532925f;
      const int drift = static_cast<int>(sinf((nowMs + i * 713UL) / 900.0f) * 5.0f +
                                         cosf(windRadians) * ecologyWind_ * 8.0f);
      int x = 12 + ((i * 29 + static_cast<int>(nowMs / 260UL) + drift) % 296);
      if (x < 8) x += 296;
      const int y = SKY_FX_TOP +
          ((i * 19 + static_cast<int>(nowMs / 125UL)) % SKY_FX_HEIGHT);
      canvas_.drawPixel(x, y, snow);
      if ((i & 3) == 0) canvas_.drawPixel(x + 1, y, snow);
    }
  }

  // Strong wind gets a few faint directional streaks. They are intentionally
  // sparse so ordinary breezes are expressed mainly by plants, bubbles and the
  // water surface rather than by drawing lines over the dashboard.
  if (ecologyWind_ > 0.58f) {
    const uint16_t gust = blend565(background, TFT_LIGHTGREY,
        static_cast<uint8_t>(25.0f + ecologyWind_ * 45.0f));
    const float angle = ecologyWindDirectionDeg_ * 0.0174532925f;
    const int dx = static_cast<int>(cosf(angle) * (7.0f + ecologyWind_ * 8.0f));
    const int dy = static_cast<int>(sinf(angle) * 3.0f);
    for (int i = 0; i < 4; ++i) {
      const int x = 30 + ((i * 83 + static_cast<int>(nowMs / 130UL)) % 250);
      const int y = 35 + ((i * 17 + static_cast<int>(nowMs / 360UL)) % 35);
      const int endY = constrain(y + dy, 29, 77);
      canvas_.drawLine(x, y, x + dx, endY, gust);
    }
  }

  // Fog appears as very subtle horizontal veils rather than opaque blocks.
  if (ecologyFog_ > 0.10f) {
    const uint16_t haze = blend565(background, TFT_LIGHTGREY,
        static_cast<uint8_t>(35.0f + ecologyFog_ * 45.0f));
    const int drift = static_cast<int>((nowMs / 220UL) % 18UL);
    for (int y = 37; y <= 73; y += 18) {
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
      canvas_.drawFastVLine(boltX, 31, 20, flash);
      canvas_.drawLine(boltX, 51, boltX - 6, 61, flash);
      canvas_.drawLine(boltX - 6, 61, boltX - 2, 76, flash);
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
  canvas_.drawPixel(x + d * 4, y - 2, TFT_WHITE);
  canvas_.drawPixel(x + d * 5, y - 2, TFT_BLACK);
  for (int i = -4; i <= 4; i += 4) {
    const int tentacleWave = static_cast<int>(sinf((nowMs + i * 90) / 210.0f) * 2.0f);
    canvas_.drawLine(rearX, y + i / 2,
                     rearX - d * (7 + pulse), y + i + tentacleWave, colour);
  }
}

void AquariumEngine::drawOctopus(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  const int sway = static_cast<int>((nowMs / 260UL) % 5UL) - 2;
  canvas_.fillCircle(x, y - 7, 7, colour);
  canvas_.fillRect(x - 7, y - 7, 15, 7, colour);
  const int eyeSide = direction > 0 ? 1 : -1;
  canvas_.drawPixel(x + eyeSide * 2, y - 9, TFT_WHITE);
  canvas_.drawPixel(x + eyeSide * 4, y - 9, TFT_WHITE);
  canvas_.drawPixel(x + eyeSide * 3, y - 9, TFT_BLACK);
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

void AquariumEngine::drawHammerhead(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  (void)nowMs;
  const int d = direction > 0 ? 1 : -1;
  // Long lean body and exaggerated T-shaped head make the silhouette readable
  // even at CYD resolution.
  const int left = direction > 0 ? x - 20 : x - 8;
  canvas_.fillRoundRect(left, y - 4, 28, 8, 4, colour);
  const int noseX = direction > 0 ? left + 28 : left;
  canvas_.drawFastVLine(noseX, y - 8, 17, colour);
  canvas_.drawFastHLine(noseX - 3, y - 8, 7, colour);
  canvas_.drawFastHLine(noseX - 3, y + 8, 7, colour);
  canvas_.drawPixel(noseX + d, y - 7, TFT_BLACK);
  canvas_.drawPixel(noseX + d, y + 7, TFT_BLACK);
  const int tailX = direction > 0 ? left : left + 28;
  canvas_.fillTriangle(tailX, y, tailX - d * 9, y - 7,
                       tailX - d * 5, y, colour);
  canvas_.fillTriangle(tailX, y, tailX - d * 9, y + 7,
                       tailX - d * 5, y, colour);
  canvas_.fillTriangle(left + 14, y - 3, left + 10, y - 11,
                       left + 19, y - 3, colour);
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

void AquariumEngine::drawSwampMonster(
    int x, int y, int8_t direction, uint16_t colour, unsigned long nowMs) {
  (void)direction;
  // Original retro gill-man / swamp-creature silhouette: deliberately generic
  // 1950s monster-movie language rather than a reproduction of any film design.
  const int bob = static_cast<int>(sinf(nowMs / 430.0f) * 1.5f);
  y += bob;
  colour = rgb565(3, 38, 46);
  const uint16_t dark = blend565(colour, TFT_BLACK, 150);
  canvas_.fillCircle(x, y - 14, 5, colour);
  canvas_.fillRoundRect(x - 5, y - 9, 11, 18, 4, colour);
  canvas_.drawLine(x - 4, y - 5, x - 11, y + 2, colour);
  canvas_.drawLine(x + 4, y - 5, x + 11, y + 2, colour);
  canvas_.drawLine(x - 3, y + 7, x - 7, y + 17, colour);
  canvas_.drawLine(x + 3, y + 7, x + 7, y + 17, colour);
  // Gill ridges and glowing eyes sell the horror-host silhouette at tiny scale.
  canvas_.drawLine(x - 5, y - 13, x - 9, y - 10, dark);
  canvas_.drawLine(x + 5, y - 13, x + 9, y - 10, dark);
  canvas_.drawPixel(x - 2, y - 15, TFT_WHITE);
  canvas_.drawPixel(x + 2, y - 15, TFT_WHITE);
  canvas_.drawPixel(x - 1, y - 15, TFT_RED);
  canvas_.drawPixel(x + 3, y - 15, TFT_RED);
  // One slow webbed-hand wave near the middle of the encounter.
  if (((nowMs / 700UL) % 5UL) < 2UL) {
    canvas_.drawLine(x + 10, y + 2, x + 14, y - 2, colour);
    canvas_.drawPixel(x + 15, y - 3, colour);
    canvas_.drawPixel(x + 15, y - 1, colour);
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
    case RareSpecies::Hammerhead:
      drawHammerhead(x, y, direction, colour, nowMs);
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
    case RareSpecies::SwampMonster:
      drawSwampMonster(x, y, direction, colour, nowMs);
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
  if (rareSpecies_ == RareSpecies::SwampMonster) baseColour = TFT_GREEN;

  const uint16_t colour = blend565(background, baseColour,
      eventType_ == EventType::LegendaryVisitor ? 235 : 220);
  drawRareVisitor(static_cast<int>(eventX_), y, eventDirection_,
                  rareSpecies_, colour, nowMs);
}


void AquariumEngine::updateCalendarEvent(
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  calendarLabel_ = "";
  if (!clock.valid) return;

  const int nowMinutes = parseClockMinutes(clock.timeText);
  const int sec = parseClockSeconds(clock.timeText);
  const String& date = clock.dateText;

  // Exact-time secret: ghost hour at 03:33. Keeping it to one minute makes it
  // something users can genuinely discover rather than permanent scenery.
  if (nowMinutes == 3 * 60 + 33) {
    calendarLabel_ = "GHOST HOUR";
    return;
  }

  // Friday the 13th gets a repeating 30-second cursed-water window every
  // thirteen minutes. This keeps the date special without dominating a whole day.
  if (isFriday13(date) && (nowMinutes % 13) == 0 && sec < 30) {
    calendarLabel_ = "CURSED WATER";
    return;
  }

  if (dateIs(date, "31 October")) {
    calendarLabel_ = "HALLOWEEN";
    return;
  }
  if (dateIs(date, "25 December")) {
    calendarLabel_ = "MERRY FISHMAS";
    return;
  }
  if (dateIs(date, "14 February")) {
    calendarLabel_ = "VALENTINE TIDE";
    return;
  }
  if (dateIs(date, "01 April")) {
    calendarLabel_ = "APRIL FOOL";
    return;
  }
  if (dateIs(date, "29 February")) {
    calendarLabel_ = "LEAP FISH DAY";
    return;
  }
  if (dateIs(date, "01 January") && nowMinutes < 10) {
    calendarLabel_ = "HAPPY NEW YEAR";
    return;
  }

  // Dawn/dusk windows use live sunrise/sunset data when available.
  int sunrise = outdoor.valid ? parseClockMinutes(outdoor.sunriseText) : -1;
  int sunset = outdoor.valid ? parseClockMinutes(outdoor.sunsetText) : -1;
  if (sunrise >= 0 && abs(nowMinutes - sunrise) <= 8) {
    calendarLabel_ = "DAWN VISITOR";
  } else if (sunset >= 0 && abs(nowMinutes - sunset) <= 8) {
    calendarLabel_ = "DUSK VISITOR";
  }
}

void AquariumEngine::drawJellyfish(unsigned long nowMs, uint16_t background) {
  if (!jellyfishEnabled_) return;
  // 90 s crossing followed by 10 min absent.
  const unsigned long cycle = 690000UL;
  const unsigned long phase = (nowMs - bootMs_) % cycle;
  if (phase >= 90000UL) return;
  const int x = -10 + static_cast<int>((phase * 340UL) / 90000UL);
  const int y = TANK_TOP + 14 + static_cast<int>(sinf(phase / 2100.0f) * 5.0f);
  const uint16_t body = blend565(background, TFT_MAGENTA, 135);
  canvas_.fillCircle(x, y, 5, body);
  canvas_.fillRect(x - 5, y, 11, 3, body);
  for (int i=-4;i<=4;i+=4) {
    int sway=static_cast<int>(sinf(nowMs/370.0f+i)*2.0f);
    canvas_.drawLine(x+i,y+3,x+i+sway,y+11,body);
  }
}

void AquariumEngine::drawHourlyBoat(const ClockReading& clock, unsigned long nowMs, uint16_t background) {
  if (!clock.valid || clock.timeText.length() < 5) return;
  const int hour = clock.timeText.substring(0,2).toInt();
  const int minute = clock.timeText.substring(3,5).toInt();
  if (minute == 0 && hour != lastBoatHour_) {
    lastBoatHour_ = hour; boatStartedMs_ = nowMs; boatDirection_ = (hour & 1) ? -1 : 1;
  }
  if (boatStartedMs_ == 0 || nowMs - boatStartedMs_ > 85000UL) return;
  unsigned long t = nowMs - boatStartedMs_;
  float progress;
  if (t < 26000UL) progress = t / 26000.0f * 0.42f;
  else if (t < 52000UL) progress = 0.42f;
  else progress = 0.42f + (t-52000UL)/33000.0f*0.68f;
  int x = boatDirection_ > 0 ? -18 + static_cast<int>(progress*356.0f) : 338 - static_cast<int>(progress*356.0f);
  const int y = 80;
  const uint16_t hull = blend565(background, TFT_ORANGE, 190);
  canvas_.fillTriangle(x-9,y,x+10,y,x+6,y+5,hull);
  canvas_.fillRect(x-3,y-5,8,5,blend565(background,TFT_WHITE,180));
  canvas_.drawFastVLine(x,y-12,7,TFT_DARKGREY);
  // Tiny fisherman and rod while paused.
  int d=boatDirection_>0?1:-1;
  canvas_.fillCircle(x+d*2,y-9,1,TFT_WHITE);
  canvas_.drawLine(x+d*2,y-8,x+d*2,y-4,TFT_DARKGREY);
  canvas_.drawLine(x+d*2,y-7,x+d*7,y-12,TFT_DARKGREY);
  canvas_.drawLine(x+d*7,y-12,x+d*14,y-1,TFT_DARKGREY);
  if (t>=26000UL && t<52000UL) canvas_.drawPixel(x+d*14,y+1,TFT_RED);
  uint16_t wake=blend565(background,TFT_CYAN,135); canvas_.drawFastHLine(x-d*14,y+4,8,wake);
}

void AquariumEngine::drawCalendarEvents(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    unsigned long nowMs,
    uint16_t background) {
  if (!clock.valid) return;

  const int nowMinutes = parseClockMinutes(clock.timeText);
  const int sec = parseClockSeconds(clock.timeText);
  const String& date = clock.dateText;
  const uint16_t pale = blend565(background, TFT_WHITE, 205);

  // 03:33 ghost fish: translucent-looking pale fish that drifts slowly through
  // the upper tank for exactly the ghost minute.
  if (nowMinutes == 3 * 60 + 33) {
    const int x = -18 + static_cast<int>((nowMs / 90UL) % 356UL);
    drawFishSprite(x, 112, 1, 2, pale, 0x3333u, nowMs, true);
    canvas_.drawPixel(x - 3, 110, TFT_BLACK);
    return;
  }

  // Friday the 13th: a tiny skeleton fish crosses during the recurring cursed
  // windows. Bones are intentionally simple so they read at CYD resolution.
  if (isFriday13(date) && (nowMinutes % 13) == 0 && sec < 30) {
    const int x = -22 + static_cast<int>((nowMs / 70UL) % 364UL);
    drawFishSprite(x, 145, 1, 2, blend565(background, TFT_LIGHTGREY, 220),
                   0x1313u, nowMs, true);
    const uint16_t bone = blend565(background, TFT_WHITE, 235);
    canvas_.drawFastHLine(x - 7, 145, 14, bone);
    for (int dx = -5; dx <= 5; dx += 5) {
      canvas_.drawFastVLine(x + dx, 142, 7, bone);
    }
    return;
  }

  if (dateIs(date, "31 October")) {
    // Small pumpkin on the tank floor plus an occasional ghostly fish.
    canvas_.fillCircle(286, 190, 7, blend565(background, TFT_ORANGE, 220));
    canvas_.fillRect(284, 180, 4, 4, blend565(background, TFT_GREEN, 210));
    canvas_.drawPixel(283, 189, TFT_BLACK);
    canvas_.drawPixel(289, 189, TFT_BLACK);
    canvas_.drawFastHLine(284, 194, 5, TFT_BLACK);
    if ((nowMinutes % 10) == 0 && sec < 35) {
      const int x = -18 + static_cast<int>((nowMs / 85UL) % 356UL);
      drawFishSprite(x, 122, 1, 2, pale, 0x1031u, nowMs, true);
    }
  }

  if (dateIs(date, "25 December")) {
    // Tiny festive lights along the hut-side floor area.
    const uint16_t colours[] = {TFT_RED, TFT_GREEN, TFT_YELLOW, TFT_CYAN};
    for (int i = 0; i < 8; ++i) {
      const int x = 205 + i * 11;
      canvas_.drawPixel(x, 187 + (i & 1), colours[i % 4]);
    }
  }

  if (dateIs(date, "14 February")) {
    // Heart bubbles rise slowly from one fixed source so the effect remains calm.
    for (int i = 0; i < 4; ++i) {
      const int y = 186 - static_cast<int>((nowMs / (130UL + i * 20UL) + i * 31) % 88UL);
      const int x = 270 + ((i & 1) ? 4 : -3);
      const uint16_t heart = blend565(background, TFT_MAGENTA, 220);
      canvas_.drawPixel(x - 1, y, heart);
      canvas_.drawPixel(x + 1, y, heart);
      canvas_.drawFastHLine(x - 2, y + 1, 5, heart);
      canvas_.drawFastHLine(x - 1, y + 2, 3, heart);
      canvas_.drawPixel(x, y + 3, heart);
    }
  }

  if (dateIs(date, "01 April")) {
    // Rubber duck: intentionally silly and very small.
    const int x = 235 + static_cast<int>(sinf(nowMs / 800.0f) * 12.0f);
    const int y = 92 + static_cast<int>(sinf(nowMs / 370.0f));
    const uint16_t yellow = blend565(background, TFT_YELLOW, 235);
    canvas_.fillCircle(x, y, 4, yellow);
    canvas_.fillCircle(x + 5, y - 4, 3, yellow);
    canvas_.drawPixel(x + 7, y - 4, TFT_ORANGE);
    canvas_.drawPixel(x + 5, y - 5, TFT_BLACK);
  }

  if (dateIs(date, "29 February")) {
    // Leap fish repeatedly hops above the surface in a slow arc.
    const float p = static_cast<float>((nowMs / 25UL) % 240UL) / 239.0f;
    const int x = 38 + static_cast<int>(p * 244.0f);
    const int y = 90 - static_cast<int>(sinf(p * 3.14159265f) * 24.0f);
    drawFishSprite(x, y, 1, 1, blend565(background, TFT_GREEN, 225),
                   0x2929u, nowMs, true);
  }

  if (dateIs(date, "01 January") && nowMinutes < 10) {
    // New-year sparkles in the sky band for the first ten minutes.
    for (int i = 0; i < 14; ++i) {
      const int x = 15 + ((i * 47 + static_cast<int>(nowMs / 180UL)) % 290);
      const int y = 35 + ((i * 23) % 40);
      canvas_.drawPixel(x, y, (i & 1) ? TFT_YELLOW : TFT_CYAN);
    }
  }

  int sunrise = outdoor.valid ? parseClockMinutes(outdoor.sunriseText) : -1;
  int sunset = outdoor.valid ? parseClockMinutes(outdoor.sunsetText) : -1;
  if ((sunrise >= 0 && abs(nowMinutes - sunrise) <= 8) ||
      (sunset >= 0 && abs(nowMinutes - sunset) <= 8)) {
    // A small ray-like dawn/dusk silhouette glides across the middle water.
    const int x = -18 + static_cast<int>((nowMs / 100UL) % 356UL);
    const int y = 132 + static_cast<int>(sinf(nowMs / 600.0f) * 3.0f);
    const uint16_t ray = blend565(background, TFT_CYAN, 150);
    canvas_.fillTriangle(x - 8, y, x, y - 5, x + 8, y, ray);
    canvas_.fillTriangle(x - 8, y, x, y + 5, x + 8, y, ray);
    canvas_.drawFastHLine(x + 8, y, 7, ray);
  }
}
