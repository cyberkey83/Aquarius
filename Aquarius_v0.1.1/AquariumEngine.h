#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "TimeService.h"
#include "WeatherService.h"

class AquariumEngine {
 public:
  explicit AquariumEngine(TFT_eSprite& canvas);

  void begin(unsigned long nowMs);
  void update(unsigned long nowMs,
              const ClockReading& clock,
              const OutdoorReading& outdoor);
  void feed(unsigned long nowMs, int screenX = 160, int screenY = 90);

  uint16_t backgroundColour(const ClockReading& clock,
                            const OutdoorReading& outdoor) const;
  void draw(const ClockReading& clock,
            const OutdoorReading& outdoor,
            unsigned long nowMs,
            uint16_t background);

  bool feedingActive(unsigned long nowMs) const;
  const char* eventLabel(unsigned long nowMs) const;

 private:
  enum class EventType : uint8_t {
    None,
    RareFish,
    Submarine,
    NocturnalFish
  };

  enum class RareSpecies : uint8_t {
    Koi,
    Jellyfish,
    AnglerFish,
    Shark,
    Squid,
    Octopus,
    Count
  };

  struct Fish {
    float x;
    float y;
    float speed;
    int8_t direction;
    uint8_t size;
    uint16_t colour;
    uint32_t phase;
  };

  TFT_eSprite& canvas_;
  Fish fish_[4];
  bool initialised_ = false;
  unsigned long lastUpdateMs_ = 0;
  unsigned long feedingUntilMs_ = 0;
  int feedingX_ = 160;
  int feedingY_ = 104;
  unsigned long nextRareEventMs_ = 0;
  unsigned long eventUntilMs_ = 0;
  EventType eventType_ = EventType::None;
  RareSpecies rareSpecies_ = RareSpecies::Koi;
  float eventX_ = -40.0f;
  int8_t eventDirection_ = 1;
  uint32_t randomState_ = 0xA51C0DEu;

  bool isNight(const ClockReading& clock) const;
  float activityFactor(const ClockReading& clock,
                       const OutdoorReading& outdoor) const;
  float preferredDepth(const OutdoorReading& outdoor) const;
  uint32_t nextRandom();
  void scheduleNextEvent(unsigned long nowMs);
  void startScheduledEvent(unsigned long nowMs,
                           const ClockReading& clock);
  void updateFish(unsigned long nowMs,
                  const ClockReading& clock,
                  const OutdoorReading& outdoor);
  void updateEvent(unsigned long nowMs,
                   const ClockReading& clock,
                   const OutdoorReading& outdoor);

  void drawWaterSurface(unsigned long nowMs,
                        uint16_t background,
                        const OutdoorReading& outdoor);
  void drawBubbles(unsigned long nowMs,
                   uint16_t background,
                   const ClockReading& clock,
                   const OutdoorReading& outdoor);
  void drawPlants(unsigned long nowMs, uint16_t background);
  void drawWeatherEffects(unsigned long nowMs,
                          uint16_t background,
                          const OutdoorReading& outdoor);
  void drawFishSchool(unsigned long nowMs,
                      uint16_t background);
  void drawFishSprite(int x,
                      int y,
                      int8_t direction,
                      uint8_t size,
                      uint16_t colour,
                      bool special = false);
  void drawFood(unsigned long nowMs, uint16_t background);
  void drawSpecialEvent(unsigned long nowMs, uint16_t background);
  void drawSubmarine(int x, int y, int8_t direction, uint16_t colour);
  void drawRareVisitor(int x,
                       int y,
                       int8_t direction,
                       RareSpecies species,
                       uint16_t colour,
                       unsigned long nowMs);
  void drawSquid(int x, int y, int8_t direction,
                 uint16_t colour, unsigned long nowMs);
  void drawOctopus(int x, int y, int8_t direction,
                   uint16_t colour, unsigned long nowMs);
};
