#pragma once

#include <TFT_eSPI.h>

namespace AquariusTheme {

constexpr uint16_t BACKGROUND = TFT_BLACK;
constexpr uint16_t PRIMARY = TFT_WHITE;
constexpr uint16_t INDOOR = TFT_CYAN;
constexpr uint16_t OUTDOOR = TFT_GREEN;
constexpr uint16_t ACCENT = TFT_YELLOW;
constexpr uint16_t MUTED = TFT_DARKGREY;
constexpr uint16_t SECONDARY = TFT_LIGHTGREY;
constexpr uint16_t ERROR_COLOUR = TFT_RED;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr int MARGIN = 10;
constexpr int CENTRE_X = SCREEN_WIDTH / 2;

}  // namespace AquariusTheme
