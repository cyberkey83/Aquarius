#include "AmbientRenderer.h"

namespace {
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
}

AmbientRenderer::AmbientRenderer(TFT_eSprite& canvas) : canvas_(canvas) {}

bool AmbientRenderer::isNight(const ClockReading& clock) const {
  if (!clock.valid || clock.timeText.length() < 2) return false;
  const int hour = clock.timeText.substring(0, 2).toInt();
  return hour < 6 || hour >= 21;
}

uint16_t AmbientRenderer::backgroundColour(
    const ClockReading& clock,
    const OutdoorReading& outdoor) const {
  if (isNight(clock)) return rgb565(0, 5, 18);
  if (!outdoor.valid) return rgb565(0, 10, 20);

  const int code = outdoor.weatherCode;
  if (code <= 1) return rgb565(0, 18, 32);                      // clear
  if (code <= 3 || code == 45 || code == 48) return rgb565(5, 13, 24); // cloudy/fog
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82))
    return rgb565(1, 8, 24);                                   // rain
  if (code >= 95) return rgb565(7, 3, 20);                     // storm
  if (code >= 71 && code <= 77) return rgb565(7, 16, 25);      // snow
  return rgb565(0, 12, 24);
}

void AmbientRenderer::draw(
    const ClockReading& clock,
    const OutdoorReading& outdoor,
    unsigned long nowMs,
    uint16_t background) {
  (void)clock;
  (void)outdoor;
  drawWaterSurface(nowMs, background);
  drawBubbles(nowMs, background);
  drawFish(nowMs, background);
  drawPlants(nowMs, background);
}

void AmbientRenderer::drawWaterSurface(
    unsigned long nowMs,
    uint16_t background) {
  const uint16_t water = blend565(background, TFT_CYAN, 100);
  const int shift = static_cast<int>((nowMs / 180UL) % 8UL);
  for (int x = -8; x < 320; x += 16) {
    const int x0 = x + shift;
    canvas_.drawFastHLine(x0, 82, 7, water);
    canvas_.drawFastHLine(x0 + 8, 83, 7, water);
  }
}

void AmbientRenderer::drawBubbles(
    unsigned long nowMs,
    uint16_t background) {
  const uint16_t bubble = blend565(background, TFT_CYAN, 145);
  constexpr int top = 88;
  constexpr int bottom = 199;
  constexpr int height = bottom - top;
  constexpr int xPositions[] = {18, 31, 55, 145, 274, 294, 307};
  constexpr unsigned long offsets[] = {0, 1100, 2600, 3900, 700, 2100, 4700};

  for (uint8_t i = 0; i < 7; ++i) {
    const unsigned long phase = (nowMs + offsets[i]) % 6200UL;
    const int y = bottom - static_cast<int>((phase * height) / 6200UL);
    const int drift = static_cast<int>((phase / 500UL + i) % 3UL) - 1;
    const int radius = (i % 3 == 0) ? 2 : 1;
    canvas_.drawCircle(xPositions[i] + drift, y, radius, bubble);
    if (radius == 2) canvas_.drawPixel(xPositions[i] + drift - 1, y - 1, bubble);
  }
}

void AmbientRenderer::drawFish(
    unsigned long nowMs,
    uint16_t background) {
  const uint16_t fishA = blend565(background, TFT_CYAN, 185);
  const uint16_t fishB = blend565(background, TFT_GREEN, 170);

  // Fish A crosses left-to-right in the clear strip between humidity and footer.
  const unsigned long phaseA = nowMs % 16000UL;
  if (phaseA < 10000UL) {
    const int x = -16 + static_cast<int>((phaseA * 352UL) / 10000UL);
    const int y = 190 + static_cast<int>((phaseA / 900UL) % 3UL) - 1;
    canvas_.drawLine(x, y, x + 4, y - 4, fishA);
    canvas_.drawLine(x, y, x + 4, y + 4, fishA);
    canvas_.drawRect(x + 4, y - 3, 8, 7, fishA);
    canvas_.drawLine(x + 12, y - 3, x + 16, y, fishA);
    canvas_.drawLine(x + 16, y, x + 12, y + 3, fishA);
    canvas_.drawPixel(x + 13, y - 1, fishA);
  }

  // Fish B crosses right-to-left less often, higher up and away from the values.
  const unsigned long phaseB = (nowMs + 9000UL) % 26000UL;
  if (phaseB < 9000UL) {
    const int x = 334 - static_cast<int>((phaseB * 350UL) / 9000UL);
    const int y = 110 + static_cast<int>((phaseB / 800UL) % 3UL) - 1;
    canvas_.drawLine(x, y, x - 4, y - 3, fishB);
    canvas_.drawLine(x, y, x - 4, y + 3, fishB);
    canvas_.drawRect(x - 12, y - 2, 8, 5, fishB);
    canvas_.drawPixel(x - 11, y - 1, fishB);
  }
}

void AmbientRenderer::drawPlants(
    unsigned long nowMs,
    uint16_t background) {
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
    canvas_.drawLine(baseX + direction, 193,
                     baseX + direction * (6 + sway), 188, plant);
  }
}
