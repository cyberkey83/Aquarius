#include "Aquarium.h"
#include <math.h>

namespace {

// Keep the animation within the dashboard body. The header and footer remain
// calm and uncluttered, while fish appear to move behind the readings.
constexpr int16_t WATER_TOP = 72;
constexpr int16_t WATER_BOTTOM = 194;

// Muted colours keep the aquarium atmospheric rather than dominant.
constexpr uint16_t FISH_BLUE = TFT_NAVY;
constexpr uint16_t FISH_CYAN = 0x03EF;
constexpr uint16_t FISH_GREY = 0x4208;
constexpr uint16_t BUBBLE_COLOUR = 0x3186;

}  // namespace

void Aquarium::begin(int16_t screenWidth, int16_t screenHeight) {
  screenWidth_ = screenWidth;
  screenHeight_ = screenHeight;
  previousUpdateMs_ = millis();

  randomSeed(
      static_cast<uint32_t>(micros()) ^
      static_cast<uint32_t>(analogRead(34)));

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    resetFish(fish_[i], i);
  }

  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    resetBubble(bubbles_[i], true);
  }
}

void Aquarium::resetFish(Fish& fish, uint8_t index) {
  fish.direction = (index % 2 == 0) ? 1 : -1;
  fish.size = (index == 0) ? 2 : 1;
  fish.shape = index % 3;
  fish.speedPixelsPerSecond = 7.0F + (index * 2.5F);

  const uint16_t colours[] = {
      FISH_BLUE,
      FISH_CYAN,
      FISH_GREY
  };
  fish.colour = colours[index % 3];

  fish.y = static_cast<float>(
      random(WATER_TOP + 12, WATER_BOTTOM - 10));

  if (fish.direction > 0) {
    fish.x = static_cast<float>(random(-80, screenWidth_ / 2));
  } else {
    fish.x = static_cast<float>(
        random(screenWidth_ / 2, screenWidth_ + 80));
  }
}

void Aquarium::resetBubble(Bubble& bubble, bool randomiseHeight) {
  bubble.x = static_cast<float>(random(8, screenWidth_ - 8));
  bubble.y = randomiseHeight
      ? static_cast<float>(random(WATER_TOP, WATER_BOTTOM))
      : static_cast<float>(WATER_BOTTOM + random(2, 18));

  bubble.risePixelsPerSecond =
      static_cast<float>(random(5, 13));
  bubble.wobblePhase =
      static_cast<float>(random(0, 628)) / 100.0F;
  bubble.radius = static_cast<uint8_t>(random(1, 3));
}

void Aquarium::update(unsigned long nowMs) {
  if (previousUpdateMs_ == 0) {
    previousUpdateMs_ = nowMs;
    return;
  }

  const unsigned long elapsedMs = nowMs - previousUpdateMs_;
  previousUpdateMs_ = nowMs;

  // Protect against a large jump after blocking network activity.
  const float deltaSeconds =
      min(elapsedMs, 250UL) / 1000.0F;

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    Fish& fish = fish_[i];

    fish.x +=
        fish.speedPixelsPerSecond *
        static_cast<float>(fish.direction) *
        deltaSeconds;

    const float margin = fish.size == 2 ? 54.0F : 36.0F;

    if (fish.direction > 0 &&
        fish.x > screenWidth_ + margin) {
      fish.x = -margin;
      fish.y = static_cast<float>(
          random(WATER_TOP + 12, WATER_BOTTOM - 10));
    }

    if (fish.direction < 0 &&
        fish.x < -margin) {
      fish.x = screenWidth_ + margin;
      fish.y = static_cast<float>(
          random(WATER_TOP + 12, WATER_BOTTOM - 10));
    }
  }

  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    Bubble& bubble = bubbles_[i];

    bubble.y -= bubble.risePixelsPerSecond * deltaSeconds;
    bubble.wobblePhase += deltaSeconds * 1.5F;

    if (bubble.y < WATER_TOP) {
      resetBubble(bubble, false);
    }
  }
}

void Aquarium::drawFish(
    TFT_eSprite& canvas,
    const Fish& fish) const {
  const int16_t x = static_cast<int16_t>(fish.x);
  const int16_t y = static_cast<int16_t>(fish.y);
  const int16_t unit = fish.size;

  // Small pixel-art silhouettes. They are intentionally simpler and dimmer
  // than the data layer.
  if (fish.direction > 0) {
    canvas.fillTriangle(
        x,
        y,
        x - (5 * unit),
        y - (3 * unit),
        x - (5 * unit),
        y + (3 * unit),
        fish.colour);

    canvas.fillTriangle(
        x - (5 * unit),
        y,
        x - (9 * unit),
        y - (4 * unit),
        x - (9 * unit),
        y + (4 * unit),
        fish.colour);

    canvas.drawPixel(
        x - (2 * unit),
        y - unit,
        TFT_LIGHTGREY);
  } else {
    canvas.fillTriangle(
        x,
        y,
        x + (5 * unit),
        y - (3 * unit),
        x + (5 * unit),
        y + (3 * unit),
        fish.colour);

    canvas.fillTriangle(
        x + (5 * unit),
        y,
        x + (9 * unit),
        y - (4 * unit),
        x + (9 * unit),
        y + (4 * unit),
        fish.colour);

    canvas.drawPixel(
        x + (2 * unit),
        y - unit,
        TFT_LIGHTGREY);
  }

  // A tiny alternate fin placement gives the three fish slight variation.
  if (fish.shape == 1) {
    canvas.drawFastVLine(
        x,
        y - (2 * unit),
        4 * unit,
        fish.colour);
  } else if (fish.shape == 2) {
    canvas.drawLine(
        x - (fish.direction * 2 * unit),
        y,
        x - (fish.direction * 4 * unit),
        y - (3 * unit),
        fish.colour);
  }
}

void Aquarium::draw(TFT_eSprite& canvas) const {
  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    const Bubble& bubble = bubbles_[i];

    const int16_t wobble =
        static_cast<int16_t>(sinf(bubble.wobblePhase) * 2.0F);
    const int16_t x =
        static_cast<int16_t>(bubble.x) + wobble;
    const int16_t y =
        static_cast<int16_t>(bubble.y);

    canvas.drawCircle(
        x,
        y,
        bubble.radius,
        BUBBLE_COLOUR);
  }

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    drawFish(canvas, fish_[i]);
  }
}
