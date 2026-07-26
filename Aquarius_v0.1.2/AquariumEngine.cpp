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
constexpr unsigned long EVENT_DURATION_MS = 18000UL;
constexpr unsigned long MIN_EVENT_GAP_MS = 4UL * 60UL * 1000UL;
constexpr unsigned long EVENT_JITTER_MS = 3UL * 60UL * 1000UL;

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
  return code >= 71 && code <= 77;
}

bool isStormCode(int code) {
  return code >= 95;
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
  const uint16_t colours[4] = {TFT_CYAN, TFT_GREEN, TFT_ORANGE, TFT_MAGENTA};
  const Personality personalities[4] = {
      Personality::Calm,
      Personality::Explorer,
      Personality::Social,
      Personality::Darting};
  const float depths[4] = {120.0f, 151.0f, 169.0f, 137.0f};

  for (uint8_t i = 0; i < 4; ++i) {
    Fish& fish = fish_[i];
    fish.x = 22.0f + i * 73.0f;
    fish.y = depths[i];
    fish.vx = 0.0f;
    fish.baseSpeed = 8.5f + i * 1.7f;
    fish.cruiseY = depths[i];
    fish.targetY = depths[i];
    fish.direction = (i % 2 == 0) ? 1 : -1;
    fish.size = (i == 3) ? 2 : 1;
    fish.colour = colours[i];
    fish.phase = nextRandom();
    fish.personality = personalities[i];
    fish.state = FishState::Cruise;
    fish.nextDecisionMs = nowMs + 1500UL + (nextRandom() % 4500UL);
    fish.stateUntilMs = 0;
  }

  lastUpdateMs_ = nowMs;
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
  float factor = isNight(clock) ? 0.55f : 1.0f;
  if (!outdoor.valid) return factor;
  const int code = outdoor.weatherCode;
  if (code <= 1) factor *= 1.18f;
  if (isRainCode(code)) factor *= 0.78f;
  if (isSnowCode(code)) factor *= 0.62f;
  if (isStormCode(code)) factor *= 0.52f;
  return factor;
}

float AquariumEngine::preferredDepth(const OutdoorReading& outdoor) const {
  if (!outdoor.valid) return 145.0f;
  if (isStormCode(outdoor.weatherCode)) return 176.0f;
  if (isRainCode(outdoor.weatherCode)) return 160.0f;
  if (isSnowCode(outdoor.weatherCode)) return 166.0f;
  return 143.0f;
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
  nextRareEventMs_ = nowMs + MIN_EVENT_GAP_MS +
      (nextRandom() % EVENT_JITTER_MS);
}

void AquariumEngine::startScheduledEvent(unsigned long nowMs,
                                         const ClockReading& clock) {
  const uint32_t choice = nextRandom() % 5u;
  if (isNight(clock) && choice == 0) {
    eventType_ = EventType::NocturnalFish;
  } else if (choice <= 1) {
    eventType_ = EventType::Submarine;
  } else {
    eventType_ = EventType::RareFish;
    rareSpecies_ = static_cast<RareSpecies>(
        nextRandom() % static_cast<uint8_t>(RareSpecies::Count));
  }

  eventDirection_ = (nextRandom() & 1u) ? 1 : -1;
  eventX_ = eventDirection_ > 0 ? -42.0f : 362.0f;
  eventY_ = 137.0f;
  eventVelocity_ = 0.0f;
  eventNextActionMs_ = nowMs + 1000UL + (nextRandom() % 2200UL);

  if (eventType_ == EventType::Submarine) eventY_ = 183.0f;
  if (eventType_ == EventType::RareFish) {
    if (rareSpecies_ == RareSpecies::Octopus) eventY_ = 190.0f;
    if (rareSpecies_ == RareSpecies::Squid) eventY_ = 135.0f;
    if (rareSpecies_ == RareSpecies::Jellyfish) eventY_ = 128.0f;
  }

  eventUntilMs_ = nowMs + EVENT_DURATION_MS;
  scheduleNextEvent(eventUntilMs_);
}

void AquariumEngine::feed(unsigned long nowMs, int screenX, int screenY) {
  feedingStartedMs_ = nowMs;
  feedingUntilMs_ = nowMs + FEED_DURATION_MS;
  feedingX_ = constrain(screenX, TANK_LEFT + 16, TANK_RIGHT - 16);
  feedingY_ = constrain(screenY, 94, 146);

  for (uint8_t i = 0; i < 4; ++i) {
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
    case EventType::RareFish: return "RARE VISITOR";
    case EventType::Submarine: return "SUBMARINE";
    case EventType::NocturnalFish: return "NIGHT VISITOR";
    default: return "";
  }
}

void AquariumEngine::update(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  if (!initialised_) begin(nowMs);
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
    for (uint8_t i = 0; i < 4; ++i) schoolY += fish_[i].y;
    schoolY /= 4.0f;
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

  const float activity = activityFactor(clock, outdoor);
  const bool feeding = feedingActive(nowMs);
  const unsigned long feedElapsed = feeding ? nowMs - feedingStartedMs_ : 0UL;

  for (uint8_t i = 0; i < 4; ++i) {
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
  (void)outdoor;
  if (eventType_ == EventType::None &&
      static_cast<long>(nowMs - nextRareEventMs_) >= 0) {
    startScheduledEvent(nowMs, clock);
  }
  if (eventType_ == EventType::None) return;
  if (static_cast<long>(nowMs - eventUntilMs_) >= 0) {
    eventType_ = EventType::None;
    return;
  }

  float dt = (nowMs - lastUpdateMs_) / 1000.0f;
  if (dt <= 0.0f || dt > 0.5f) dt = 0.1f;

  float targetSpeed = eventType_ == EventType::Submarine ? 18.0f : 24.0f;

  if (eventType_ == EventType::RareFish) {
    switch (rareSpecies_) {
      case RareSpecies::Octopus:
        targetSpeed = 6.5f;
        // Octopus pauses and creeps instead of gliding continuously.
        if (static_cast<long>(nowMs - eventNextActionMs_) >= 0) {
          eventVelocity_ = (nextRandom() % 4u == 0u) ? 0.0f : targetSpeed;
          eventNextActionMs_ = nowMs + 900UL + (nextRandom() % 1700UL);
        }
        eventY_ = 190.0f + sinf(nowMs / 850.0f) * 1.2f;
        break;
      case RareSpecies::Squid:
        // Pulsed propulsion: calm glide punctuated by fast darts.
        targetSpeed = ((nowMs / 950UL) % 4UL == 0UL) ? 48.0f : 24.0f;
        eventY_ = 135.0f + sinf(nowMs / 470.0f) * 7.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) * clampFloat(dt * 5.5f, 0.0f, 1.0f);
        break;
      case RareSpecies::Jellyfish:
        targetSpeed = 11.0f;
        eventY_ = 128.0f + sinf(nowMs / 640.0f) * 8.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) * clampFloat(dt * 2.0f, 0.0f, 1.0f);
        break;
      case RareSpecies::Shark:
        targetSpeed = 31.0f;
        eventVelocity_ += (targetSpeed - eventVelocity_) * clampFloat(dt * 4.0f, 0.0f, 1.0f);
        break;
      default:
        eventVelocity_ += (targetSpeed - eventVelocity_) * clampFloat(dt * 3.0f, 0.0f, 1.0f);
        break;
    }
  } else {
    eventVelocity_ += (targetSpeed - eventVelocity_) * clampFloat(dt * 3.0f, 0.0f, 1.0f);
  }

  if (eventType_ == EventType::RareFish && rareSpecies_ == RareSpecies::Octopus) {
    if (eventVelocity_ <= 0.1f && static_cast<long>(eventNextActionMs_ - nowMs) > 0) return;
  }

  eventX_ += eventVelocity_ * eventDirection_ * dt;
}

uint16_t AquariumEngine::backgroundColour(
    const ClockReading& clock,
    const OutdoorReading& outdoor) const {
  if (isNight(clock)) return rgb565(0, 5, 18);
  if (!outdoor.valid) return rgb565(0, 10, 20);
  const int code = outdoor.weatherCode;
  if (code <= 1) return rgb565(0, 18, 32);
  if (code <= 3 || code == 45 || code == 48) return rgb565(5, 13, 24);
  if (isRainCode(code)) return rgb565(1, 8, 24);
  if (isStormCode(code)) return rgb565(7, 3, 20);
  if (isSnowCode(code)) return rgb565(7, 16, 25);
  return rgb565(0, 12, 24);
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
  drawFood(nowMs, background);
  drawFishSchool(nowMs, background);
  drawSpecialEvent(nowMs, background);
}

void AquariumEngine::drawWaterSurface(
    unsigned long nowMs,
    uint16_t background,
    const OutdoorReading& outdoor) {
  const bool storm = outdoor.valid && isStormCode(outdoor.weatherCode);
  const uint16_t water = blend565(background, TFT_CYAN, storm ? 155 : 95);
  const int speed = storm ? 75 : 180;
  const int shift = static_cast<int>((nowMs / speed) % 8UL);
  const int amplitude = storm ? 2 : 1;
  for (int x = -8; x < 320; x += 16) {
    const int x0 = x + shift;
    canvas_.drawFastHLine(x0, 82, 7, water);
    canvas_.drawFastHLine(x0 + 8, 82 + amplitude, 7, water);
  }
}

void AquariumEngine::drawBubbles(
    unsigned long nowMs,
    uint16_t background,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  const uint16_t bubble = blend565(background, TFT_CYAN, 145);
  float rate = activityFactor(clock, outdoor);
  if (rate < 0.45f) rate = 0.45f;
  constexpr int xs[] = {18, 31, 55, 145, 274, 294, 307, 118};
  constexpr uint32_t offsets[] = {0, 1100, 2600, 3900, 700, 2100, 4700, 3200};
  const unsigned long period = static_cast<unsigned long>(6200.0f / rate);
  for (uint8_t i = 0; i < 8; ++i) {
    const unsigned long phase = (nowMs + offsets[i]) % period;
    const int y = TANK_BOTTOM - static_cast<int>((phase * (TANK_BOTTOM - 88)) / period);
    const int drift = static_cast<int>((phase / 500UL + i) % 3UL) - 1;
    const int radius = (i % 3 == 0) ? 2 : 1;
    canvas_.drawCircle(xs[i] + drift, y, radius, bubble);
  }
}

void AquariumEngine::drawPlants(unsigned long nowMs, uint16_t background) {
  const uint16_t plant = blend565(background, TFT_GREEN, 155);
  const int sway = static_cast<int>((nowMs / 550UL) % 5UL) - 2;
  for (int side = 0; side < 2; ++side) {
    const int baseX = side == 0 ? 10 : 309;
    const int direction = side == 0 ? 1 : -1;
    canvas_.drawLine(baseX, 201, baseX + sway, 180, plant);
    canvas_.drawLine(baseX + direction * 4, 201,
                     baseX + direction * (8 + sway), 185, plant);
    canvas_.drawLine(baseX - direction * 2, 201,
                     baseX - direction * (5 - sway), 190, plant);
  }
}

void AquariumEngine::drawWeatherEffects(
    unsigned long nowMs,
    uint16_t background,
    const OutdoorReading& outdoor) {
  if (!outdoor.valid) return;
  if (isRainCode(outdoor.weatherCode)) {
    const uint16_t rain = blend565(background, TFT_BLUE, 150);
    for (uint8_t i = 0; i < 9; ++i) {
      const int x = 14 + ((i * 37 + nowMs / 90) % 292);
      const int y = 88 + ((i * 23 + nowMs / 55) % 102);
      canvas_.drawPixel(x, y, rain);
      canvas_.drawPixel(x, y + 1, rain);
    }
  }
  if (isSnowCode(outdoor.weatherCode)) {
    const uint16_t snow = blend565(background, TFT_WHITE, 175);
    for (uint8_t i = 0; i < 11; ++i) {
      const int x = 12 + ((i * 29 + nowMs / 150) % 296);
      const int y = 88 + ((i * 19 + nowMs / 115) % 105);
      canvas_.drawPixel(x, y, snow);
    }
  }
  if (isStormCode(outdoor.weatherCode) && (nowMs % 9000UL) < 120UL) {
    const uint16_t flash = blend565(background, TFT_WHITE, 100);
    canvas_.drawFastVLine(305, 88, 35, flash);
    canvas_.drawLine(305, 123, 299, 132, flash);
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
  for (uint8_t i = 0; i < 4; ++i) {
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
  } else {
    const uint16_t colour = eventType_ == EventType::NocturnalFish
                                ? blend565(background, TFT_MAGENTA, 225)
                                : blend565(background,
                                    rareSpecies_ == RareSpecies::Squid
                                        ? TFT_CYAN : TFT_ORANGE,
                                    220);
    drawRareVisitor(static_cast<int>(eventX_), y, eventDirection_,
                    eventType_ == EventType::NocturnalFish
                        ? RareSpecies::AnglerFish : rareSpecies_,
                    colour, nowMs);
  }
}
