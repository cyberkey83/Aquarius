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
  void configure(uint8_t fishCount, uint8_t bubbleLevel, uint8_t plantLevel,
                 uint8_t plantLength, uint8_t animationLevel, bool weatherEffects,
                 uint8_t eventFrequency, bool jellyfishEnabled);

  uint16_t backgroundColour(const ClockReading& clock,
                            const OutdoorReading& outdoor) const;
  void draw(const ClockReading& clock,
            const OutdoorReading& outdoor,
            unsigned long nowMs,
            uint16_t background);

  bool feedingActive(unsigned long nowMs) const;
  const char* eventLabel(unsigned long nowMs) const;

  // Hidden showcase support. Scene numbers are intentionally kept as a small
  // stable API so the Arduino sketch can drive every visitor on demand.
  void setDemoScene(uint8_t scene, unsigned long nowMs);
  void clearDemo(unsigned long nowMs);
  const char* demoSceneLabel() const;

 private:
  enum class EventType : uint8_t {
    None,
    Visitor,
    Submarine,
    NocturnalVisitor,
    LegendaryVisitor
  };

  enum class RareSpecies : uint8_t {
    Koi,
    Jellyfish,
    AnglerFish,
    Shark,
    Hammerhead,
    Squid,
    Octopus,
    PufferFish,
    Seahorse,
    Whale,
    SwampMonster,
    Count
  };

  enum class EventRarity : uint8_t {
    Uncommon,
    Rare,
    VeryRare,
    Legendary
  };

  enum class Personality : uint8_t {
    Calm,
    Explorer,
    Social,
    Darting
  };

  enum class FishState : uint8_t {
    Cruise,
    Turn,
    FeedApproach,
    FeedFrenzy,
    Recover
  };

  struct Fish {
    float x;
    float y;
    float vx;
    float baseSpeed;
    float cruiseY;
    float targetY;
    int8_t direction;
    uint8_t size;
    uint16_t colour;
    uint32_t phase;
    Personality personality;
    FishState state;
    unsigned long nextDecisionMs;
    unsigned long stateUntilMs;
  };

  TFT_eSprite& canvas_;
  static constexpr uint8_t MAX_RESIDENT_FISH = 10;
  Fish fish_[MAX_RESIDENT_FISH];
  bool initialised_ = false;
  unsigned long lastUpdateMs_ = 0;
  unsigned long feedingStartedMs_ = 0;
  unsigned long feedingUntilMs_ = 0;
  int feedingX_ = 160;
  int feedingY_ = 104;
  unsigned long nextRareEventMs_ = 0;
  unsigned long eventUntilMs_ = 0;
  unsigned long eventStartedMs_ = 0;
  unsigned long bootMs_ = 0;
  EventType eventType_ = EventType::None;
  EventRarity eventRarity_ = EventRarity::Uncommon;
  RareSpecies rareSpecies_ = RareSpecies::Koi;
  RareSpecies lastSpecies_ = RareSpecies::Count;
  RareSpecies previousSpecies_ = RareSpecies::Count;
  EventType lastEventType_ = EventType::None;
  float eventX_ = -40.0f;
  float eventY_ = 137.0f;
  float eventVelocity_ = 0.0f;
  int8_t eventDirection_ = 1;
  unsigned long eventNextActionMs_ = 0;
  uint32_t randomState_ = 0xA51C0DEu;
  uint8_t fishCount_ = 4;
  uint8_t bubbleLevel_ = 2;
  uint8_t plantLevel_ = 2;
  uint8_t plantLength_ = 5;
  uint8_t animationLevel_ = 2;
  uint8_t eventFrequency_ = 2;
  bool weatherEffectsEnabled_ = true;
  bool jellyfishEnabled_ = true;
  unsigned long jellyCycleStartMs_ = 0;
  int lastBoatHour_ = -1;
  unsigned long boatStartedMs_ = 0;
  int8_t boatDirection_ = 1;
  String calendarLabel_;
  uint8_t demoScene_ = 255;

  // Weather Ecology state. These values move gradually toward live-weather
  // targets so the aquarium changes mood rather than snapping between modes.
  bool ecologySeeded_ = false;
  float ecologyDaylight_ = 1.0f;
  float ecologyRain_ = 0.0f;
  float ecologySnow_ = 0.0f;
  float ecologyStorm_ = 0.0f;
  float ecologyFog_ = 0.0f;
  float ecologyCloud_ = 0.0f;
  float ecologyWind_ = 0.0f;
  float ecologyWindDirectionDeg_ = 0.0f;
  float ecologyActivity_ = 1.0f;
  float ecologyDepth_ = 143.0f;
  float ecologySurface_ = 0.35f;
  float ecologyBubbleRate_ = 1.0f;
  float ecologyBgR_ = 0.0f;
  float ecologyBgG_ = 12.0f;
  float ecologyBgB_ = 24.0f;

  bool isNight(const ClockReading& clock) const;
  float activityFactor(const ClockReading& clock,
                       const OutdoorReading& outdoor) const;
  float preferredDepth(const OutdoorReading& outdoor) const;
  float daylightFactor(const ClockReading& clock,
                       const OutdoorReading& outdoor) const;
  void updateEcology(unsigned long nowMs,
                     const ClockReading& clock,
                     const OutdoorReading& outdoor);
  uint32_t nextRandom();
  float randomUnit();
  float personalitySpeedFactor(Personality personality) const;
  float personalityWander(Personality personality) const;
  void scheduleNextEvent(unsigned long nowMs);
  void startScheduledEvent(unsigned long nowMs,
                           const ClockReading& clock,
                           const OutdoorReading& outdoor);
  EventRarity chooseEventRarity(unsigned long nowMs) ;
  RareSpecies chooseRareSpecies(const ClockReading& clock,
                                const OutdoorReading& outdoor,
                                EventRarity rarity);
  bool speciesEligible(RareSpecies species,
                       const ClockReading& clock,
                       const OutdoorReading& outdoor,
                       EventRarity rarity) const;
  unsigned long eventDurationFor(RareSpecies species,
                                 EventType type) const;
  float eventBaseSpeed(RareSpecies species, EventType type) const;
  void chooseFishTarget(Fish& fish,
                        uint8_t index,
                        unsigned long nowMs,
                        const OutdoorReading& outdoor);
  void updateFish(unsigned long nowMs,
                  const ClockReading& clock,
                  const OutdoorReading& outdoor);
  void updateEvent(unsigned long nowMs,
                   const ClockReading& clock,
                   const OutdoorReading& outdoor);

  void drawSkyAtmosphere(const ClockReading& clock, const OutdoorReading& outdoor,
                         unsigned long nowMs, uint16_t background);
  void drawCelestial(const ClockReading& clock, const OutdoorReading& outdoor,
                     unsigned long nowMs, uint16_t background);
  void drawWaterSurface(unsigned long nowMs,
                        uint16_t background,
                        const OutdoorReading& outdoor);
  void drawBubbles(unsigned long nowMs,
                   uint16_t background,
                   const ClockReading& clock,
                   const OutdoorReading& outdoor);
  void drawParallax(unsigned long nowMs, uint16_t background);
  void drawSeabed(unsigned long nowMs, uint16_t background);
  void drawPlants(unsigned long nowMs, uint16_t background);
  void drawCrab(unsigned long nowMs, uint16_t background);
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
                      uint32_t phase,
                      unsigned long nowMs,
                      bool special = false);
  void drawFood(unsigned long nowMs, uint16_t background);
  void drawSpecialEvent(unsigned long nowMs, uint16_t background);
  void updateCalendarEvent(const ClockReading& clock, const OutdoorReading& outdoor);
  void drawJellyfish(unsigned long nowMs, uint16_t background);
  void drawHourlyBoat(const ClockReading& clock, unsigned long nowMs, uint16_t background);
  void drawCalendarEvents(const ClockReading& clock, const OutdoorReading& outdoor,
                          unsigned long nowMs, uint16_t background);
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
  void drawPufferFish(int x, int y, int8_t direction,
                      uint16_t colour, unsigned long nowMs);
  void drawSeahorse(int x, int y, int8_t direction,
                    uint16_t colour, unsigned long nowMs);
  void drawHammerhead(int x, int y, int8_t direction,
                      uint16_t colour, unsigned long nowMs);
  void drawWhale(int x, int y, int8_t direction,
                 uint16_t colour, unsigned long nowMs);
  void drawSwampMonster(int x, int y, int8_t direction,
                        uint16_t colour, unsigned long nowMs);
};
