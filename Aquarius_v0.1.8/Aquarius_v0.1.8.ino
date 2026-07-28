/*
 * Aquarius v0.1.8 — Environment & Atmosphere
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
#include <WiFi.h>

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

String bootStatus = "Starting Aquarius...";

void presentCanvas();

void renderBootFrame() {
  displayManager.drawBootScreen(bootStatus, millis());
  presentCanvas();
}

void animateBootStatus(const String& status, unsigned long durationMs) {
  bootStatus = status;
  const unsigned long start = millis();
  do {
    renderBootFrame();
    delay(35);
  } while (millis() - start < durationMs);
}

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
      a.plantLength, a.animationLevel, a.weatherEffects, a.eventFrequency);
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
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString("AQUARIUS SETTINGS", 10, 8, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.setTextDatum(TR_DATUM);
  canvas.drawString(settingsPage == 0 ? "AQUARIUM" :
                    (settingsPage == 1 ? "SYSTEM" : "ABOUT"), 310, 8, 2);
  canvas.drawFastHLine(8, 29, 304, TFT_DARKGREY);

  auto adjustableRow = [&](int y, const String& name, const String& value) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(name, 14, y, 2);

    // The visible arrows are deliberately small, but their touch zones are
    // much wider so adjustment remains comfortable on a 2.8-inch CYD.
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString("<", 205, y + 8, 2);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.drawString(value, 258, y + 8, 2);
    canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString(">", 307, y + 8, 2);
  };

  auto infoRow = [&](int y, const String& name, const String& value) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(name, 14, y, 2);
    canvas.setTextDatum(TR_DATUM);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.drawString(value, 302, y, 2);
  };

  auto compactInfoRow = [&](int y, const String& name, const String& value) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(name, 14, y, 1);
    canvas.setTextDatum(TR_DATUM);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.drawString(value, 306, y, 1);
  };

  if (settingsPage == 0) {
    adjustableRow(38, "Fish", String(a.fishCount));
    adjustableRow(62, "Bubbles", String(a.bubbleLevel));
    adjustableRow(86, "Plants", String(a.plantLevel));
    adjustableRow(110, "Plant length", String(a.plantLength));
    adjustableRow(134, "Animation", String(a.animationLevel));
    adjustableRow(158, "Weather FX", a.weatherEffects ? "ON" : "OFF");
    adjustableRow(182, "Rare events", String(a.eventFrequency));
  } else if (settingsPage == 1) {
    adjustableRow(42, "Brightness", String(a.brightness));
    adjustableRow(70, "Night bright", String(a.nightBrightness));
    adjustableRow(98, "Auto dim", a.dimMinutes ? String(a.dimMinutes) + " min" : "NEVER");
    adjustableRow(126, "Clock", a.use24Hour ? "24 HOUR" : "12 HOUR");
    adjustableRow(154, "Temperature", a.fahrenheit ? "F" : "C");

    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("Reset defaults", 14, 182, 2);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.drawString("RESET", 258, 190, 2);
  } else {
    String ssid = timeService.wifiConnected() ? WiFi.SSID() : "--";
    if (ssid.length() > 25) ssid = ssid.substring(0, 25);
    const String ip = timeService.wifiConnected() ? WiFi.localIP().toString() : "--";
    const String mac = WiFi.macAddress();
    compactInfoRow(39,  "Firmware", "v0.1.8");
    compactInfoRow(59,  "Wi-Fi", timeService.wifiConnected() ? "CONNECTED" : "OFFLINE");
    compactInfoRow(79,  "SSID", ssid);
    compactInfoRow(99,  "IP", ip);
    compactInfoRow(119, "MAC", mac);
    compactInfoRow(139, "Sensor", String(indoorSensor.barometricName()));
    compactInfoRow(159, "Location", AQUARIUS_LOCATION_NAME);
    compactInfoRow(179, "Settings", "NVS PERSISTENT");
  }

  canvas.drawFastHLine(8, 211, 304, TFT_DARKGREY);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("< BACK", 48, 226, 2);
  canvas.drawString("PAGE >", 272, 226, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString(settingsPage < 2 ? "tap < or >" : "system information", 160, 226, 1);
  presentCanvas();
}

int discreteIndex(const uint8_t* values, int count, uint8_t current) {
  for (int i = 0; i < count; ++i) {
    if (values[i] == current) return i;
  }
  return 0;
}

void changeSettingAt(int x, int y) {
  AquariusSettings& a = settingsManager.values();
  const int row = settingsPage == 0 ? (y - 32) / 24 : (y - 34) / 28;
  const int maxRow = settingsPage == 0 ? 6 : 5;
  if (row < 0 || row > maxRow || settingsPage > 1) return;

  // Large invisible hit targets around the small arrows.
  const bool decrease = (x >= 176 && x <= 229);
  const bool increase = (x >= 282 && x <= 319);

  // Reset is an action rather than a numeric field, so the central RESET label
  // is tappable across the right half of the row.
  if (settingsPage == 1 && row == 5) {
    if (x >= 190) {
      settingsManager.resetDefaults();
      applyAquariumSettings();
      applyBrightness();
      drawSettings();
    }
    return;
  }

  if (!decrease && !increase) return;
  const int direction = increase ? 1 : -1;

  if (settingsPage == 0) {
    if (row == 0) a.fishCount = constrain((int)a.fishCount + direction, 1, 10);
    if (row == 1) a.bubbleLevel = constrain((int)a.bubbleLevel + direction, 0, 10);
    if (row == 2) a.plantLevel = constrain((int)a.plantLevel + direction, 0, 10);
    if (row == 3) a.plantLength = constrain((int)a.plantLength + direction, 1, 10);
    if (row == 4) a.animationLevel = constrain((int)a.animationLevel + direction, 0, 3);
    if (row == 5) a.weatherEffects = !a.weatherEffects;
    if (row == 6) a.eventFrequency = constrain((int)a.eventFrequency + direction, 0, 3);
    applyAquariumSettings();
  } else if (settingsPage == 1) {
    if (row == 0) {
      a.brightness = constrain((int)a.brightness + direction * 20, 40, 255);
      applyBrightness();
    }
    if (row == 1) {
      a.nightBrightness = constrain((int)a.nightBrightness + direction * 20, 20, 220);
      applyBrightness();
    }
    if (row == 2) {
      const uint8_t vals[] = {0, 2, 5, 10, 20, 30, 60};
      int i = discreteIndex(vals, 7, a.dimMinutes);
      i = constrain(i + direction, 0, 6);
      a.dimMinutes = vals[i];
    }
    if (row == 3) {
      a.use24Hour = !a.use24Hour;
      applyAquariumSettings();
    }
    if (row == 4) {
      a.fahrenheit = !a.fahrenheit;
      applyAquariumSettings();
    }
  }

  settingsManager.save();
  drawSettings();
}

void pollTouch(unsigned long nowMs) {
  const bool touched = touchscreen.touched();
  if (touched && !touchWasDown) {
    TS_Point point = touchscreen.getPoint();
    if (point.z >= AQUARIUS_TOUCH_MIN_PRESSURE) {
      touchDownX = constrain(map(point.x, AQUARIUS_TOUCH_MIN_X, AQUARIUS_TOUCH_MAX_X,
                                 0, AquariusTheme::SCREEN_WIDTH - 1),
                             0, AquariusTheme::SCREEN_WIDTH - 1);
      touchDownY = constrain(map(point.y, AQUARIUS_TOUCH_MIN_Y, AQUARIUS_TOUCH_MAX_Y,
                                 0, AquariusTheme::SCREEN_HEIGHT - 1),
                             0, AquariusTheme::SCREEN_HEIGHT - 1);
      touchDownMs = nowMs;
      lastInteractionMs = nowMs;
      applyBrightness();
    }
  }

  if (!touched && touchWasDown && touchDownMs != 0) {
    const unsigned long held = nowMs - touchDownMs;
    if (settingsOpen) {
      if (touchDownY > 208 && touchDownX < 105) {
        settingsOpen = false;
        settingsManager.save();
      } else if (touchDownY > 208 && touchDownX > 215) {
        settingsPage = (settingsPage + 1) % 3;
        drawSettings();
      } else {
        changeSettingAt(touchDownX, touchDownY);
      }
    } else if (held >= AQUARIUS_LONG_PRESS_MS) {
      settingsOpen = true;
      settingsPage = 0;
      drawSettings();
      Serial.println("Settings          : opened");
    } else if (held >= 40) {
      displayManager.feedFish(nowMs, touchDownX, touchDownY);
      Serial.printf("Aquarium          : feeding triggered at %d,%d\n",
                    touchDownX, touchDownY);
    }
    touchDownMs = 0;
  }
  touchWasDown = touched;
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
  Serial.println(" AQUARIUS v0.1.8");
  Serial.println(" Aquarium Engine");
  Serial.println("========================================");

  initialiseDisplay();
  settingsManager.begin();
  applyBrightness();
  lastInteractionMs = millis();
  initialiseTouch();
  displayManager.beginAquarium(millis());
  applyAquariumSettings();

  animateBootStatus("Initialising display...", 800UL);

  bootStatus = "Starting indoor sensor...";
  renderBootFrame();
  indoorSensor.begin(Wire, I2C_SDA_PIN, I2C_SCL_PIN);
  animateBootStatus(
      indoorSensor.hasAnySensor()
          ? String("Sensor ready: ") + indoorSensor.barometricName()
          : "Indoor sensor unavailable",
      700UL);

  bootStatus = "Connecting to Wi-Fi...";
  renderBootFrame();
  timeService.begin(renderBootFrame);
  animateBootStatus(
      timeService.wifiConnected() ? "Wi-Fi connected" : "Wi-Fi unavailable",
      600UL);

  bootStatus = "Synchronising time...";
  const unsigned long syncStart = millis();
  while (!timeService.timeValid() && millis() - syncStart < 8000UL) {
    timeService.update();
    renderBootFrame();
    delay(80);
  }
  animateBootStatus(timeService.timeValid() ? "Clock synchronised" : "Time sync pending", 550UL);

  bootStatus = "Loading weather...";
  renderBootFrame();
  weatherService.begin();
  animateBootStatus(weatherService.reading().valid ? "Weather ready" : "Weather pending", 650UL);
  animateBootStatus("Aquarius ready", 550UL);

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
