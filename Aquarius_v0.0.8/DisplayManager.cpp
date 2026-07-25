#include "DisplayManager.h"

#include "AquariusConfig.h"
#include "AquariusTheme.h"

namespace {
constexpr unsigned long FOOTER_PAGE_INTERVAL_MS = 5000UL;
constexpr uint8_t FOOTER_PAGE_COUNT = 4;
}

DisplayManager::DisplayManager(TFT_eSprite& canvas) : canvas_(canvas) {}

void DisplayManager::clear() {
  canvas_.fillSprite(AquariusTheme::BACKGROUND);
}

void DisplayManager::drawBootScreen() {
  clear();
  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::INDOOR, AquariusTheme::BACKGROUND);
  canvas_.drawString("AQUARIUS", AquariusTheme::CENTRE_X, 52, 4);

  canvas_.setTextColor(AquariusTheme::PRIMARY, AquariusTheme::BACKGROUND);
  canvas_.drawString("Desktop Companion", AquariusTheme::CENTRE_X, 84, 2);

  canvas_.setTextColor(AquariusTheme::MUTED, AquariusTheme::BACKGROUND);
  canvas_.drawString("v0.0.8", AquariusTheme::CENTRE_X, 108, 2);

  drawBootLine("Initialising sensors...", 152, AquariusTheme::SECONDARY);
}

void DisplayManager::drawBootLine(
    const String& text,
    int y,
    uint16_t colour) {
  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(colour, AquariusTheme::BACKGROUND);
  canvas_.fillRect(8, y - 11, 304, 24, AquariusTheme::BACKGROUND);
  canvas_.drawString(text, AquariusTheme::CENTRE_X, y, 2);
}

void DisplayManager::drawDashboard(
    const ClockReading& clock,
    const IndoorReading& indoor,
    const OutdoorReading& outdoor,
    bool wifiConnected,
    const char* pressureTrendLabel,
    const char* pressureTrendSymbol,
    unsigned long nowMs) {
  clear();
  drawHeader(clock, wifiConnected);
  drawWeatherPanel(indoor, outdoor);
  drawFooter(
      clock,
      indoor,
      outdoor,
      pressureTrendLabel,
      pressureTrendSymbol,
      nowMs);
}

void DisplayManager::drawHeader(
    const ClockReading& clock,
    bool wifiConnected) {
  canvas_.setTextDatum(TL_DATUM);
  canvas_.setTextColor(AquariusTheme::PRIMARY, AquariusTheme::BACKGROUND);
  canvas_.drawString("AQUARIUS", 10, 7, 2);

  canvas_.setTextDatum(TR_DATUM);
  canvas_.setTextColor(
      wifiConnected ? AquariusTheme::OUTDOOR : AquariusTheme::ERROR_COLOUR,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(wifiConnected ? "ONLINE" : "OFFLINE", 310, 7, 2);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::INDOOR, AquariusTheme::BACKGROUND);
  canvas_.drawString(clock.timeText, AquariusTheme::CENTRE_X, 38, 4);

  canvas_.setTextColor(
      clock.valid ? AquariusTheme::SECONDARY : AquariusTheme::ACCENT,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(clock.dateText, AquariusTheme::CENTRE_X, 64, 2);

  canvas_.drawFastHLine(10, 79, 300, AquariusTheme::MUTED);
}

String DisplayManager::temperatureText(float value, bool valid) const {
  return valid ? String(value, 1) + " C" : "--.- C";
}

String DisplayManager::humidityText(float value, bool valid) const {
  return valid ? String(value, 0) + "%" : "--%";
}

String DisplayManager::compactCondition(const char* condition) const {
  if (condition == nullptr || condition[0] == '\0') return "WAITING";

  String text(condition);
  text.toUpperCase();
  if (text.length() > 18) {
    text = text.substring(0, 18);
  }
  return text;
}

void DisplayManager::drawWeatherPanel(
    const IndoorReading& indoor,
    const OutdoorReading& outdoor) {
  canvas_.drawFastVLine(160, 91, 102, AquariusTheme::MUTED);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::SECONDARY, AquariusTheme::BACKGROUND);
  canvas_.drawString("INDOOR", 80, 96, 2);
  canvas_.drawString("OUTDOOR", 240, 96, 2);

  canvas_.setTextColor(
      indoor.temperatureValid ? AquariusTheme::INDOOR : AquariusTheme::MUTED,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(
      temperatureText(indoor.temperatureC, indoor.temperatureValid),
      80,
      126,
      4);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::OUTDOOR : AquariusTheme::MUTED,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(
      temperatureText(outdoor.temperatureC, outdoor.valid),
      240,
      126,
      4);

  canvas_.setTextColor(AquariusTheme::MUTED, AquariusTheme::BACKGROUND);
  canvas_.drawString("HUMIDITY", 80, 153, 1);
  canvas_.drawString("HUMIDITY", 240, 153, 1);

  canvas_.setTextColor(
      indoor.humidityValid ? AquariusTheme::INDOOR : AquariusTheme::MUTED,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(
      humidityText(indoor.humidityPct, indoor.humidityValid),
      80,
      171,
      2);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::OUTDOOR : AquariusTheme::MUTED,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(
      humidityText(outdoor.humidityPct, outdoor.valid),
      240,
      171,
      2);

  canvas_.setTextColor(
      outdoor.valid ? AquariusTheme::SECONDARY : AquariusTheme::MUTED,
      AquariusTheme::BACKGROUND);
  canvas_.drawString(
      outdoor.valid ? compactCondition(outdoor.conditionText) : "WAITING",
      240,
      188,
      1);

  canvas_.drawFastHLine(10, 202, 300, AquariusTheme::MUTED);
}

void DisplayManager::drawFooter(
    const ClockReading& clock,
    const IndoorReading& indoor,
    const OutdoorReading& outdoor,
    const char* pressureTrendLabel,
    const char* pressureTrendSymbol,
    unsigned long nowMs) {
  const uint8_t page =
      static_cast<uint8_t>((nowMs / FOOTER_PAGE_INTERVAL_MS) % FOOTER_PAGE_COUNT);

  canvas_.setTextDatum(MC_DATUM);
  canvas_.setTextColor(AquariusTheme::ACCENT, AquariusTheme::BACKGROUND);

  String footer;
  switch (page) {
    case 0:
      if (indoor.pressureValid) {
        footer = String(indoor.pressureHpa, 1) + " hPa";
        if (pressureTrendSymbol != nullptr && pressureTrendSymbol[0] != '\0') {
          footer += " ";
          footer += pressureTrendSymbol;
        }
        if (pressureTrendLabel != nullptr && pressureTrendLabel[0] != '\0') {
          footer += "  ";
          footer += pressureTrendLabel;
        }
      } else {
        footer = "PRESSURE UNAVAILABLE";
      }
      break;

    case 1:
      footer = outdoor.valid
                   ? String("SUNRISE ") + outdoor.sunriseText
                   : "SUNRISE --:--";
      break;

    case 2:
      footer = outdoor.valid
                   ? String("SUNSET ") + outdoor.sunsetText
                   : "SUNSET --:--";
      break;

    default:
      canvas_.setTextColor(AquariusTheme::SECONDARY, AquariusTheme::BACKGROUND);
      footer = clock.valid
                   ? String("SYNCED ") + clock.timeText
                   : String(AQUARIUS_LOCATION_NAME);
      break;
  }

  canvas_.drawString(footer, AquariusTheme::CENTRE_X, 220, 2);
}
