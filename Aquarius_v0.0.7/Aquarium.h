#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class Aquarium {
 public:
  void begin(int16_t screenWidth, int16_t screenHeight);
  void update(unsigned long nowMs);
  void draw(TFT_eSprite& canvas) const;

 private:
  struct Fish {
    float x;
    float y;
    float speedPixelsPerSecond;
    int8_t direction;
    uint8_t size;
    uint16_t colour;
    uint8_t shape;
  };

  struct Bubble {
    float x;
    float y;
    float risePixelsPerSecond;
    float wobblePhase;
    uint8_t radius;
  };

  static constexpr uint8_t FISH_COUNT = 3;
  static constexpr uint8_t BUBBLE_COUNT = 7;

  int16_t screenWidth_ = 320;
  int16_t screenHeight_ = 240;
  unsigned long previousUpdateMs_ = 0;

  Fish fish_[FISH_COUNT];
  Bubble bubbles_[BUBBLE_COUNT];

  void resetFish(Fish& fish, uint8_t index);
  void resetBubble(Bubble& bubble, bool randomiseHeight);
  void drawFish(TFT_eSprite& canvas, const Fish& fish) const;
};
