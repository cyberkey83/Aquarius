/*
 * Aquarius v0.1.5 — Settings
 *
 * Focus:
 *   - cleaner information hierarchy
 *   - restrained colour palette
 *   - reusable DisplayManager
 *   - rotating single-line footer
 *   - weather/time responsive aquatic background
 *   - animated water surface, bubbles, fish and plants
 *   - clean dashboard remains the primary focus
 */
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#include "AquariusConfig.h"
#include "AquariusTheme.h"
#include "DisplayManager.h"
#include "IndoorSensor.h"
#include "PressureTrend.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "SettingsManager.h"

namespace {

constexpr uint8_t I2C_SDA_PIN = 22;
constexpr uint8_t I2C_SCL_PIN = 27;
constexpr uint8_t BACKLIGHT_PIN = 21;
constexpr unsigned long DISPLAY_INTERVAL_MS = 100UL;

TFT_eSPI tft;
TFT_eSprite canvas(&tft);

IndoorSensor indoorSensor;
TimeService timeService;
WeatherService weatherService;
PressureTrend pressureTrend;
DisplayManager displayManager(canvas);
SettingsManager settingsManager;

bool settingsOpen = false;
uint8_t settingsPage = 0;
unsigned long touchDownMs = 0;
int touchDownX = 0;
int touchDownY = 0;
unsigned long lastInteractionMs = 0;

unsigned long lastSensorUpdateMs = 0;
unsigned long lastClockUpdateMs = 0;
unsigned long lastDisplayUpdateMs = 0;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touchscreen(
    AQUARIUS_TOUCH_CS_PIN,
    AQUARIUS_TOUCH_IRQ_PIN);

unsigned long lastTouchMs = 0;
bool touchWasDown = false;

void presentCanvas() {
  canvas.pushSprite(0, 0);
}

void initialiseDisplay() {
  ledcAttach(BACKLIGHT_PIN, 5000, 8);
  ledcWrite(BACKLIGHT_PIN, AQUARIUS_DEFAULT_BRIGHTNESS);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  canvas.setColorDepth(8);
  if (canvas.createSprite(
          AquariusTheme::SCREEN_WIDTH,
          AquariusTheme::SCREEN_HEIGHT) == nullptr) {
    Serial.println("ERROR: Unable to allocate display sprite");
    while (true) delay(1000);
  }

  canvas.fillSprite(AquariusTheme::BACKGROUND);
}

void renderDashboard(unsigned long nowMs) {
  displayManager.updateAquarium(
      nowMs, timeService.reading(), weatherService.reading());
  displayManager.drawDashboard(
      timeService.reading(),
      indoorSensor.reading(),
      weatherService.reading(),
      timeService.wifiConnected(),
      pressureTrend.label(),
      pressureTrend.symbol(),
      nowMs);
  presentCanvas();
}

void initialiseTouch() {
  touchSpi.begin(
      AQUARIUS_TOUCH_CLK_PIN,
      AQUARIUS_TOUCH_MISO_PIN,
      AQUARIUS_TOUCH_MOSI_PIN,
      AQUARIUS_TOUCH_CS_PIN);
  touchscreen.begin(touchSpi);
  touchscreen.setRotation(1);
  Serial.println("Touch controller  : ready");
}

void applyAquariumSettings() {
  const AquariusSettings& a = settingsManager.values();
  displayManager.configureAquarium(a.fishCount, a.bubbleLevel, a.plantLevel,
      a.animationLevel, a.weatherEffects, a.eventFrequency);
  displayManager.configureDisplay(a.use24Hour, a.fahrenheit);
}

void applyBrightness(bool dimmed = false) {
  const AquariusSettings& a = settingsManager.values();
  uint8_t base = a.brightness;
  const ClockReading& c = timeService.reading();
  if (c.valid && c.timeText.length() >= 2) {
    const int h = c.timeText.substring(0,2).toInt();
    if (h >= 22 || h < 7) base = a.nightBrightness;
  }
  uint8_t value = dimmed ? min<uint8_t>(base, 45) : base;
  ledcWrite(BACKLIGHT_PIN, value);
}

void drawSettings() {
  const AquariusSettings& a = settingsManager.values();
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK); canvas.setTextDatum(TL_DATUM);
  canvas.drawString("AQUARIUS SETTINGS", 10, 8, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK); canvas.setTextDatum(TR_DATUM);
  canvas.drawString(settingsPage == 0 ? "AQUARIUM" : (settingsPage == 1 ? "SYSTEM" : "ABOUT"), 310, 8, 2);
  canvas.drawFastHLine(8, 29, 304, TFT_DARKGREY);
  canvas.setTextDatum(TL_DATUM);
  auto row=[&](int y,const String& name,const String& value){
    canvas.setTextColor(TFT_WHITE,TFT_BLACK); canvas.drawString(name,14,y,2);
    canvas.setTextDatum(TR_DATUM); canvas.setTextColor(TFT_CYAN,TFT_BLACK); canvas.drawString(value,302,y,2); canvas.setTextDatum(TL_DATUM);
  };
  if (settingsPage == 0) {
    row(42,"Fish",String(a.fishCount)); row(70,"Bubbles",String(a.bubbleLevel));
    row(98,"Plants",String(a.plantLevel)); row(126,"Animation",String(a.animationLevel));
    row(154,"Weather FX",a.weatherEffects?"ON":"OFF"); row(182,"Rare events",String(a.eventFrequency));
  } else if (settingsPage == 1) {
    row(42,"Brightness",String(a.brightness)); row(70,"Night bright",String(a.nightBrightness));
    row(98,"Auto dim",a.dimMinutes?String(a.dimMinutes)+" min":"NEVER");
    row(126,"Clock",a.use24Hour?"24 HOUR":"12 HOUR"); row(154,"Temperature",a.fahrenheit?"F":"C");
    row(182,"Reset defaults",">");
  } else {
    row(48,"Firmware","v0.1.5"); row(78,"Wi-Fi",timeService.wifiConnected()?"CONNECTED":"OFFLINE");
    row(108,"Sensor",String(indoorSensor.barometricName())); row(138,"Location",AQUARIUS_LOCATION_NAME);
    row(168,"Settings","NVS PERSISTENT");
  }
  canvas.drawFastHLine(8, 211, 304, TFT_DARKGREY);
  canvas.setTextDatum(MC_DATUM); canvas.setTextColor(TFT_YELLOW,TFT_BLACK);
  canvas.drawString("< BACK",48,226,2); canvas.drawString("PAGE >",272,226,2);
  canvas.setTextColor(TFT_DARKGREY,TFT_BLACK); canvas.drawString("tap row to change",160,226,1);
  presentCanvas();
}

void changeSettingAt(int y) {
  AquariusSettings& a = settingsManager.values();
  int row = (y - 34) / 28;
  if (row < 0 || row > 5) return;
  if (settingsPage == 0) {
    if(row==0) a.fishCount = a.fishCount >= 10 ? 1 : a.fishCount+1;
    if(row==1) a.bubbleLevel = (a.bubbleLevel+1)%11;
    if(row==2) a.plantLevel = (a.plantLevel+1)%11;
    if(row==3) a.animationLevel = (a.animationLevel+1)%4;
    if(row==4) a.weatherEffects = !a.weatherEffects;
    if(row==5) a.eventFrequency = (a.eventFrequency+1)%4;
    applyAquariumSettings();
  } else if (settingsPage == 1) {
    if(row==0) { a.brightness = a.brightness >= 250 ? 80 : a.brightness+20; applyBrightness(); }
    if(row==1) a.nightBrightness = a.nightBrightness >= 210 ? 30 : a.nightBrightness+30;
    if(row==2) { const uint8_t vals[]={0,2,5,10,20,30,60}; int i=0; while(i<7&&vals[i]!=a.dimMinutes)i++; a.dimMinutes=vals[(i+1)%7]; }
    if(row==3) { a.use24Hour=!a.use24Hour; applyAquariumSettings(); }
    if(row==4) { a.fahrenheit=!a.fahrenheit; applyAquariumSettings(); }
    if(row==5) { settingsManager.resetDefaults(); applyAquariumSettings(); applyBrightness(); }
  }
  settingsManager.save(); drawSettings();
}

void pollTouch(unsigned long nowMs) {
  const bool touched = touchscreen.touched();
  if (touched && !touchWasDown) {
    TS_Point point=touchscreen.getPoint();
    if(point.z>=AQUARIUS_TOUCH_MIN_PRESSURE){
      touchDownX=constrain(map(point.x,AQUARIUS_TOUCH_MIN_X,AQUARIUS_TOUCH_MAX_X,0,AquariusTheme::SCREEN_WIDTH-1),0,AquariusTheme::SCREEN_WIDTH-1);
      touchDownY=constrain(map(point.y,AQUARIUS_TOUCH_MIN_Y,AQUARIUS_TOUCH_MAX_Y,0,AquariusTheme::SCREEN_HEIGHT-1),0,AquariusTheme::SCREEN_HEIGHT-1);
      touchDownMs=nowMs; lastInteractionMs=nowMs; applyBrightness();
    }
  }
  if (!touched && touchWasDown && touchDownMs != 0) {
    const unsigned long held=nowMs-touchDownMs;
    if(settingsOpen){
      if(touchDownY>208 && touchDownX<105){ settingsOpen=false; settingsManager.save(); }
      else if(touchDownY>208 && touchDownX>215){ settingsPage=(settingsPage+1)%3; drawSettings(); }
      else changeSettingAt(touchDownY);
    } else if(held>=AQUARIUS_LONG_PRESS_MS){
      settingsOpen=true; settingsPage=0; drawSettings(); Serial.println("Settings          : opened");
    } else if(held>=40){
      displayManager.feedFish(nowMs,touchDownX,touchDownY);
      Serial.printf("Aquarium          : feeding triggered at %d,%d\n",touchDownX,touchDownY);
    }
    touchDownMs=0;
  }
  touchWasDown=touched;
}

void printStatus() {
  const IndoorReading& indoor = indoorSensor.reading();
  const OutdoorReading& outdoor = weatherService.reading();

  Serial.println("----------------------------------------");
  Serial.printf(
      "Wi-Fi             : %s\n",
      timeService.wifiConnected() ? "connected" : "disconnected");
  Serial.printf("Indoor sensor     : %s\n", indoorSensor.barometricName());

  if (indoor.temperatureValid) {
    Serial.printf("Indoor temperature: %.2f C\n", indoor.temperatureC);
  }
  if (indoor.humidityValid) {
    Serial.printf("Indoor humidity   : %.2f %%RH\n", indoor.humidityPct);
  } else {
    Serial.println("Indoor humidity   : unavailable");
  }
  if (indoor.pressureValid) {
    Serial.printf("Indoor pressure   : %.2f hPa\n", indoor.pressureHpa);
    Serial.printf(
        "Pressure trend    : %s (%+.2f hPa, %u samples)\n",
        pressureTrend.label(),
        pressureTrend.changeHpa(),
        pressureTrend.sampleCount());
  }

  if (outdoor.valid) {
    Serial.printf("Outdoor temp      : %.2f C\n", outdoor.temperatureC);
    Serial.printf("Outdoor humidity  : %.0f %%RH\n", outdoor.humidityPct);
    Serial.printf("Outdoor condition : %s\n", outdoor.conditionText);
    Serial.printf(
        "Sunrise / sunset  : %s / %s\n",
        outdoor.sunriseText,
        outdoor.sunsetText);
  } else {
    Serial.println("Outdoor weather   : unavailable");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AQUARIUS v0.1.5");
  Serial.println(" Aquarium Engine");
  Serial.println("========================================");

  initialiseDisplay();
  settingsManager.begin();
  applyBrightness();
  lastInteractionMs = millis();
  initialiseTouch();
  displayManager.beginAquarium(millis());
  applyAquariumSettings();
  displayManager.drawBootScreen();
  presentCanvas();

  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);
  displayManager.drawBootLine(
      String("Sensor: ") + indoorSensor.barometricName(),
      152,
      indoorSensor.hasAnySensor()
          ? AquariusTheme::OUTDOOR
          : AquariusTheme::ERROR_COLOUR);
  presentCanvas();
  delay(400);

  displayManager.drawBootLine(
      "Connecting Wi-Fi...",
      178,
      AquariusTheme::SECONDARY);
  presentCanvas();
  timeService.begin();

  displayManager.drawBootLine(
      timeService.wifiConnected() ? "Wi-Fi connected" : "Wi-Fi unavailable",
      178,
      timeService.wifiConnected()
          ? AquariusTheme::OUTDOOR
          : AquariusTheme::ERROR_COLOUR);
  presentCanvas();

  const unsigned long syncStart = millis();
  while (!timeService.timeValid() && millis() - syncStart < 8000UL) {
    timeService.update();
    delay(250);
  }

  displayManager.drawBootLine(
      "Fetching outdoor weather...",
      204,
      AquariusTheme::SECONDARY);
  presentCanvas();
  weatherService.begin();

  if (indoorSensor.reading().pressureValid) {
    pressureTrend.begin(indoorSensor.reading().pressureHpa);
  }

  const unsigned long now = millis();
  renderDashboard(now);
  printStatus();

  lastSensorUpdateMs = now;
  lastClockUpdateMs = now;
  lastDisplayUpdateMs = now;
}

void loop() {
  const unsigned long now = millis();
  pollTouch(now);

  if (now - lastClockUpdateMs >= AQUARIUS_CLOCK_INTERVAL_MS) {
    lastClockUpdateMs = now;
    timeService.update();
    weatherService.update();
  }

  if (now - lastSensorUpdateMs >= AQUARIUS_SENSOR_INTERVAL_MS) {
    lastSensorUpdateMs = now;
    indoorSensor.update();

    if (indoorSensor.reading().pressureValid) {
      pressureTrend.update(indoorSensor.reading().pressureHpa);
    }

    printStatus();
  }

  if (!settingsOpen && now - lastDisplayUpdateMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayUpdateMs = now;
    renderDashboard(now);
  }

  const AquariusSettings& activeSettings = settingsManager.values();
  if (!settingsOpen && activeSettings.dimMinutes > 0 &&
      now - lastInteractionMs > (unsigned long)activeSettings.dimMinutes * 60000UL) {
    applyBrightness(true);
  }

  delay(1);
}
