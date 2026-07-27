#include "DisplayManager.h"

#include <math.h>

#include "AquariusConfig.h"
#include "AquariusTheme.h"

namespace {
constexpr unsigned long FOOTER_PAGE_INTERVAL_MS = 5000UL;
constexpr uint8_t FOOTER_PAGE_COUNT = 5;
}

DisplayManager::DisplayManager(TFT_eSprite& canvas)
    : canvas_(canvas), aquarium_(canvas) {}

void DisplayManager::beginAquarium(unsigned long nowMs) {
  aquarium_.begin(nowMs);
}

void DisplayManager::updateAquarium(
    unsigned long nowMs,
    const ClockReading& clock,
    const OutdoorReading& outdoor) {
  aquarium_.update(nowMs, clock, outdoor);
}

void DisplayManager::feedFish(unsigned long nowMs, int screenX, int screenY) {
  aquarium_.feed(nowMs, screenX, screenY);
}

bool DisplayManager::feedingActive(unsigned long nowMs) const {
  return aquarium_.feedingActive(nowMs);
}

void DisplayManager::configureDisplay(bool use24Hour, bool fahrenheit) {
  use24Hour_ = use24Hour;
  fahrenheit_ = fahrenheit;
}

String DisplayManager::clockText(const ClockReading& clock) const {
  if (use24Hour_ || !clock.valid || clock.timeText.length() < 5) return clock.timeText;
  int hour = clock.timeText.substring(0, 2).toInt();
  const String suffix = hour >= 12 ? " PM" : " AM";
  hour %= 12; if (hour == 0) hour = 12;
  String rest = clock.timeText.substring(2);
  return String(hour) + rest + suffix;
}

void DisplayManager::configureAquarium(uint8_t fishCount, uint8_t bubbleLevel, uint8_t plantLevel,
                                       uint8_t plantLength, uint8_t animationLevel, bool weatherEffects,
                                       uint8_t eventFrequency) {
  aquarium_.configure(fishCount, bubbleLevel, plantLevel, plantLength, animationLevel,
                      weatherEffects, eventFrequency);
}

void DisplayManager::clear(
    const ClockReading* clock,
    const OutdoorReading* outdoor) {
  background_ = (clock != nullptr && outdoor != nullptr)
                    ? aquarium_.backgroundColour(*clock, *outdoor)
                    : AquariusTheme::BACKGROUND;
  canvas_.fillSprite(background_);
}

void DisplayManager::drawBootScreen(const String& status, unsigned long nowMs) {
  clear();

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::INDOOR, background_);
  canvas_.drawString("AQUARIUS", AquariusTheme::CENTRE_X, 38, 4);

  canvas_.setTextColor(AquariusTheme::PRIMARY, background_);
  canvas_.drawString("Desktop Companion", AquariusTheme::CENTRE_X, 66, 2);

  canvas_.setTextColor(AquariusTheme::MUTED, background_);
  canvas_.drawString("v0.1.7", AquariusTheme::CENTRE_X, 87, 2);

  // One cheerful boot fish continuously crosses the display. Keeping the
  // animation self-contained means it can continue while Wi-Fi is connecting.
  constexpr unsigned long FISH_PASS_MS = 4300UL;
  if (bootAnimationStartMs_ == 0) bootAnimationStartMs_ = nowMs;
  const unsigned long phase = (nowMs - bootAnimationStartMs_) % FISH_PASS_MS;
  const int fishX = -22 + static_cast<int>((phase * 364UL) / FISH_PASS_MS);
  const int fishY = 120 + static_cast<int>(sinf(nowMs / 380.0f) * 4.0f);
  const bool tailUp = ((nowMs / 130UL) & 1UL) != 0;
  const uint16_t fishColour = AquariusTheme::OUTDOOR;
  const uint16_t finColour = AquariusTheme::ACCENT;

  // Draw fins/tail first, then the body. This keeps the dorsal fin visibly
  // attached instead of having its base apparently clipped by the body layer.
  canvas_.fillTriangle(fishX - 1, fishY - 4,
                       fishX - 6, fishY - 11,
                       fishX + 5, fishY - 5, finColour);
  canvas_.fillTriangle(fishX - 8, fishY,
                       fishX - 17, fishY - (tailUp ? 7 : 4),
                       fishX - 17, fishY + (tailUp ? 4 : 7), finColour);
  canvas_.fillRoundRect(fishX - 10, fishY - 5, 20, 10, 5, fishColour);
  canvas_.fillTriangle(fishX, fishY + 3,
                       fishX - 4, fishY + 9,
                       fishX + 4, fishY + 4, finColour);
  canvas_.drawPixel(fishX + 6, fishY - 2, TFT_BLACK);

  canvas_.setTextColor(AquariusTheme::SECONDARY, background_);
  canvas_.drawString(status, AquariusTheme::CENTRE_X, 154, 2);

  // Tiny animated loading dots reinforce that Aquarius is still alive during
  // slower Wi-Fi/NTP startup phases.
  String dots;
  const uint8_t dotCount = static_cast<uint8_t>((nowMs / 400UL) % 4UL);
  for (uint8_t i = 0; i < dotCount; ++i) dots += ".";
  canvas_.setTextColor(AquariusTheme::MUTED, background_);
  canvas_.drawString(dots, AquariusTheme::CENTRE_X, 173, 2);

  canvas_.drawFastHLine(44, 188, 232, AquariusTheme::MUTED);
  canvas_.setTextColor(AquariusTheme::ACCENT, background_);
  canvas_.drawString("cyberkey83", AquariusTheme::CENTRE_X, 202, 2);
  canvas_.setTextColor(AquariusTheme::MUTED, background_);
  canvas_.drawString("github.com/cyberkey83/Aquarius", AquariusTheme::CENTRE_X, 224, 1);
}

void DisplayManager::drawDashboard(
    const ClockReading& clock,
    const IndoorReading& indoor,
    const OutdoorReading& outdoor,
    bool wifiConnected,
    const char* pressureTrendLabel,
    const char* pressureTrendSymbol,
    unsigned long nowMs) {
  clear(&clock, &outdoor);
  aquarium_.draw(clock, outdoor, nowMs, background_);
  drawHeader(clock, wifiConnected);
  drawWeatherPanel(indoor, outdoor);
  drawFooter(clock, indoor, outdoor, pressureTrendLabel, pressureTrendSymbol, nowMs);
}

void DisplayManager::drawHeader(
    const ClockReading& clock,
    bool wifiConnected) {
  canvas_.setTextDatum(TL_DATUM);
  canvas_.setTextColor(AquariusTheme::PRIMARY, background_);
  canvas_.drawString("AQUARIUS", 10, 7, 2);

  canvas_.setTextDatum(TR_DATUM);
  canvas_.setTextColor(
      wifiConnected ? AquariusTheme::OUTDOOR : AquariusTheme::ERROR_COLOUR,
      background_);
  canvas_.drawString(wifiConnected ? "ONLINE" : "OFFLINE", 310, 7, 2);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::INDOOR, background_);
  canvas_.drawString(clockText(clock), AquariusTheme::CENTRE_X, 38, 4);

  canvas_.setTextColor(
      clock.valid ? AquariusTheme::SECONDARY : AquariusTheme::ACCENT,
      background_);
  canvas_.drawString(clock.dateText, AquariusTheme::CENTRE_X, 64, 2);

  canvas_.drawFastHLine(10, 79, 300, AquariusTheme::MUTED);
}

String DisplayManager::temperatureText(float value, bool valid) const {
  if (!valid) return fahrenheit_ ? "--.- F" : "--.- C";
  const float shown = fahrenheit_ ? (value * 9.0f / 5.0f + 32.0f) : value;
  return String(shown, 1) + (fahrenheit_ ? " F" : " C");
}

String DisplayManager::humidityText(float value, bool valid) const {
  return valid ? String(value, 0) + "%" : "--%";
}

String DisplayManager::compactCondition(const char* condition) const {
  if (condition == nullptr || condition[0] == '\0') return "WAITING";
  String text(condition);
  text.toUpperCase();
  if (text.length() > 18) text = text.substring(0, 18);
  return text;
}

void DisplayManager::drawWeatherPanel(
    const IndoorReading& indoor,
    const OutdoorReading& outdoor) {
  canvas_.drawFastVLine(160, 91, 106, AquariusTheme::MUTED);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::SECONDARY, background_);
  canvas_.drawString("INDOOR", 80, 96, 2);
  canvas_.drawString("OUTDOOR", 240, 96, 2);

  canvas_.setTextColor(
      indoor.temperatureValid ? AquariusTheme::INDOOR : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      temperatureText(indoor.temperatureC, indoor.temperatureValid), 80, 124, 4);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::OUTDOOR : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      temperatureText(outdoor.temperatureC, outdoor.valid), 240, 124, 4);

  canvas_.setTextColor(AquariusTheme::MUTED, background_);
  canvas_.drawString("HUMIDITY", 80, 149, 1);
  canvas_.drawString("HUMIDITY", 240, 149, 1);

  canvas_.setTextColor(
      indoor.humidityValid ? AquariusTheme::INDOOR : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      humidityText(indoor.humidityPct, indoor.humidityValid), 80, 164, 2);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::OUTDOOR : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      humidityText(outdoor.humidityPct, outdoor.valid), 240, 164, 2);

  canvas_.setTextColor(AquariusTheme::MUTED, background_);
  canvas_.drawString("PRESSURE", 80, 180, 1);
  canvas_.setTextColor(
      indoor.pressureValid ? AquariusTheme::INDOOR : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      indoor.pressureValid ? String(indoor.pressureHpa, 1) + " hPa"
                           : "----.- hPa",
      80, 193, 2);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::SECONDARY : AquariusTheme::MUTED,
      background_);
  canvas_.drawString(
      outdoor.valid ? compactCondition(outdoor.conditionText) : "WAITING",
      240, 188, 1);

  canvas_.drawFastHLine(10, 204, 300, AquariusTheme::MUTED);
}

void DisplayManager::drawFooter(
    const ClockReading& clock,
    const IndoorReading& indoor,
    const OutdoorReading& outdoor,
    const char* pressureTrendLabel,
    const char* pressureTrendSymbol,
    unsigned long nowMs) {
  const uint8_t page = static_cast<uint8_t>(
      (nowMs / FOOTER_PAGE_INTERVAL_MS) % FOOTER_PAGE_COUNT);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::ACCENT, background_);

  String footer;
  switch (page) {
    case 0:
      footer = "PRESSURE ";
      if (pressureTrendSymbol != nullptr && pressureTrendSymbol[0] != '\0') {
        footer += pressureTrendSymbol;
        footer += " ";
      }
      footer += (pressureTrendLabel != nullptr && pressureTrendLabel[0] != '\0')
                    ? pressureTrendLabel
                    : "LEARNING";
      break;
    case 1:
      footer = outdoor.valid ? String("SUNRISE ") + outdoor.sunriseText
                             : "SUNRISE --:--";
      break;
    case 2:
      footer = outdoor.valid ? String("SUNSET ") + outdoor.sunsetText
                             : "SUNSET --:--";
      break;
    case 3:
      canvas_.setTextColor(AquariusTheme::SECONDARY, background_);
      footer = aquarium_.feedingActive(nowMs)
                   ? "FEEDING TIME"
                   : "TAP SCREEN TO FEED";
      break;
    default:
      canvas_.setTextColor(AquariusTheme::SECONDARY, background_);
      footer = clock.valid ? String("SYNCED ") + clock.timeText
                           : String(AQUARIUS_LOCATION_NAME);
      break;
  }
  canvas_.drawString(footer, AquariusTheme::CENTRE_X, 220, 2);
}
