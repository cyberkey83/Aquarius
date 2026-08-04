/*
 * AtmoQuarium v0.1.14 — Refinement Build
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
bool diagnosticsOpen = false;
bool demoOpen = false;
bool demoAuto = true;
uint8_t demoScene = 0;
unsigned long demoSceneStartedMs = 0;
constexpr uint8_t DEMO_SCENE_COUNT = 34;
constexpr unsigned long DEMO_SCENE_DURATION_MS = 9000UL;
bool wifiKeyboardOpen = false;
bool wifiKeyboardShift = true;
String wifiEditSsid;
String wifiEditPassword;
static constexpr uint8_t MAX_WIFI_RESULTS = 5;
String wifiScanSsids[MAX_WIFI_RESULTS];
int32_t wifiScanRssi[MAX_WIFI_RESULTS] = {0};
uint8_t wifiScanCount = 0;
String wifiStatusText = "tap RESCAN";
unsigned long lastRenderDurationUs = 0;
unsigned long lastDiagnosticsDrawMs = 0;
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

String bootStatus = "Starting AtmoQuarium...";

void presentCanvas();
void drawSettings();
void drawWifiKeyboard();
void enterDemo(unsigned long nowMs);
void setDemoScene(uint8_t scene, unsigned long nowMs);
void exitDemo(unsigned long nowMs);

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

bool backlightPwmReady = false;

void writeBacklight(uint8_t duty) {
  if (!backlightPwmReady) {
    backlightPwmReady = ledcAttach(BACKLIGHT_PIN, 5000, 8);
    pinMode(BACKLIGHT_PIN, OUTPUT);
    Serial.println(backlightPwmReady ? "Backlight PWM    : ready"
                                     : "Backlight PWM    : attach failed");
  }
  if (backlightPwmReady) {
    ledcWrite(BACKLIGHT_PIN, duty);
  } else {
    // Fallback retains a usable display if PWM setup fails.
    digitalWrite(BACKLIGHT_PIN, duty > 0 ? HIGH : LOW);
  }
}

void initialiseDisplay() {
  writeBacklight(AQUARIUS_DEFAULT_BRIGHTNESS);

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

void buildDemoReadings(ClockReading& c, OutdoorReading& o) {
  c = timeService.reading();
  o = weatherService.reading();
  c.valid = true;
  o.valid = true;
  strlcpy(o.sunriseText, "06:30", sizeof(o.sunriseText));
  strlcpy(o.sunsetText, "20:30", sizeof(o.sunsetText));
  o.windSpeedKmh = 8.0f; o.windDirectionDeg = 250.0f; o.weatherCode = 0;
  strlcpy(o.conditionText, "DEMO", sizeof(o.conditionText));

  if (demoScene == 3) c.timeText = "06:32:00";
  else if (demoScene == 4) c.timeText = "20:26:00";
  else if (demoScene == 5 || (demoScene >= 13 && demoScene <= 23) || demoScene == 24) c.timeText = "23:30:00";
  else c.timeText = "12:00:00";

  if (demoScene == 6) { o.weatherCode = 3; strlcpy(o.conditionText,"CLOUDY",24); }
  if (demoScene == 7) { o.weatherCode = 45; strlcpy(o.conditionText,"FOG",24); }
  if (demoScene == 8) { o.weatherCode = 63; strlcpy(o.conditionText,"RAIN",24); }
  if (demoScene == 9) { o.weatherCode = 95; o.windSpeedKmh = 48; strlcpy(o.conditionText,"STORM",24); }
  if (demoScene == 10) { o.weatherCode = 73; strlcpy(o.conditionText,"SNOW",24); }
  if (demoScene == 11) { o.weatherCode = 1; o.windSpeedKmh = 70; o.windDirectionDeg = 270; strlcpy(o.conditionText,"HIGH WIND",24); }

  if (demoScene == 24) { c.timeText = "03:33:10"; c.dateText = "29 July 2026"; }
  if (demoScene == 25) { c.timeText = "13:00:10"; c.dateText = "Friday 13 November 2026"; }
  if (demoScene == 26) c.dateText = "31 October 2026";
  if (demoScene == 27) c.dateText = "25 December 2026";
  if (demoScene == 28) c.dateText = "14 February 2026";
  if (demoScene == 29) c.dateText = "01 April 2026";
  if (demoScene == 30) c.dateText = "29 February 2028";
  if (demoScene == 31) { c.dateText = "01 January 2027"; c.timeText = "00:04:00"; }
  if (demoScene == 32) c.timeText = "06:31:00";
}

void drawDemoOverlay() {
  canvas.fillRect(0, 210, 320, 30, TFT_BLACK);
  canvas.drawFastHLine(0, 210, 320, TFT_DARKGREY);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString(String("DEMO: ") + displayManager.demoSceneLabel(), 160, 217, 1);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("< PREV", 47, 231, 1);
  canvas.drawString(demoAuto ? "AUTO ON" : "AUTO OFF", 160, 231, 1);
  canvas.drawString("NEXT >", 273, 231, 1);
}

void renderDashboard(unsigned long nowMs) {
  const unsigned long renderStartUs = micros();
  ClockReading demoClock;
  OutdoorReading demoOutdoor;
  const ClockReading* clock = &timeService.reading();
  const OutdoorReading* outdoor = &weatherService.reading();
  if (demoOpen) { buildDemoReadings(demoClock, demoOutdoor); clock = &demoClock; outdoor = &demoOutdoor; }
  displayManager.updateAquarium(nowMs, *clock, *outdoor);
  displayManager.drawDashboard(*clock, indoorSensor.reading(), *outdoor,
      timeService.wifiConnected(), pressureTrend.label(), pressureTrend.symbol(), nowMs);
  if (demoOpen) drawDemoOverlay();
  presentCanvas();
  lastRenderDurationUs = micros() - renderStartUs;
}

void setDemoScene(uint8_t scene, unsigned long nowMs) {
  demoScene = scene % DEMO_SCENE_COUNT;
  demoSceneStartedMs = nowMs;
  displayManager.setDemoScene(demoScene, nowMs);
  renderDashboard(nowMs);
}

void enterDemo(unsigned long nowMs) {
  diagnosticsOpen = false; settingsOpen = false; demoOpen = true; demoAuto = true;
  setDemoScene(0, nowMs);
}

void exitDemo(unsigned long nowMs) {
  demoOpen = false; displayManager.clearDemo(nowMs); settingsOpen = true; settingsPage = 4; drawSettings();
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
      a.plantLength, a.animationLevel, a.weatherEffects, a.eventFrequency, a.jellyfishEnabled);
  displayManager.configureDisplay(a.use24Hour, a.fahrenheit);
}

bool backlightDimmed = false;

void applyBrightness(bool dimmed = false) {
  const AquariusSettings& a = settingsManager.values();
  uint8_t base = a.brightness;
  const ClockReading& c = timeService.reading();
  if (c.valid && c.timeText.length() >= 2) {
    const int h = c.timeText.substring(0, 2).toInt();
    if (h >= 22 || h < 7) base = a.nightBrightness;
  }

  // Use a very low but non-zero duty when dimmed. The previous 45/255 level
  // was too subtle on some CYD backlight transistor batches.
  const uint8_t value = dimmed ? 12 : base;
  writeBacklight(value);
  backlightDimmed = dimmed;
  Serial.printf("DIM: %s, backlight duty %u/255
",
                dimmed ? "ACTIVE" : "AWAKE", value);
}

void drawDiagnostics() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.drawString("ATMOQUARIUM DIAGNOSTICS", 10, 8, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.setTextDatum(TR_DATUM);
  canvas.drawString("SECRET TEST SCREEN", 310, 8, 1);
  canvas.drawFastHLine(8, 29, 304, TFT_DARKGREY);

  auto row = [&](int y, const String& name, const String& value) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(name, 14, y, 1);
    canvas.setTextDatum(TR_DATUM);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.drawString(value, 306, y, 1);
  };

  const unsigned long upSec = millis() / 1000UL;
  char uptime[20];
  snprintf(uptime, sizeof(uptime), "%02lu:%02lu:%02lu",
           upSec / 3600UL, (upSec / 60UL) % 60UL, upSec % 60UL);
  const float renderLoad = min(100.0f, (lastRenderDurationUs / 1000.0f) /
                                      static_cast<float>(DISPLAY_INTERVAL_MS) * 100.0f);
  unsigned long weatherAgeSec = 0;
  if (weatherService.lastSuccessMs() > 0)
    weatherAgeSec = (millis() - weatherService.lastSuccessMs()) / 1000UL;

  row(36,  "CPU frequency", String(ESP.getCpuFreqMHz()) + " MHz");
  row(51,  "Render load", String(renderLoad, 1) + "%");
  row(66,  "Free heap", String(ESP.getFreeHeap() / 1024UL) + " KB");
  row(81,  "Min free heap", String(ESP.getMinFreeHeap() / 1024UL) + " KB");
  row(96,  "Max alloc block", String(ESP.getMaxAllocHeap() / 1024UL) + " KB");
  row(111, "Sketch size", String(ESP.getSketchSize() / 1024UL) + " KB");
  row(126, "Free sketch", String(ESP.getFreeSketchSpace() / 1024UL) + " KB");
  row(141, "Uptime", String(uptime));
  row(156, "Wi-Fi RSSI", timeService.wifiConnected() ? String(WiFi.RSSI()) + " dBm" : "OFFLINE");
  row(171, "Sensor poll", String(settingsManager.values().sensorIntervalSec) + " sec");
  row(186, "Weather age", weatherService.reading().valid ? String(weatherAgeSec) + " sec" : "--");
  row(201, "Sensor", indoorSensor.barometricName());

  canvas.drawFastHLine(8, 211, 304, TFT_DARKGREY);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("TAP TO RETURN", 160, 226, 2);
  presentCanvas();
  lastDiagnosticsDrawMs = millis();
}

void scanWifiNetworks() {
  wifiStatusText = "scanning...";
  drawSettings();
  WiFi.mode(WIFI_STA);
  const int found = WiFi.scanNetworks(false, true);
  wifiScanCount = 0;
  if (found <= 0) {
    wifiStatusText = "no networks found";
    WiFi.scanDelete();
    return;
  }
  // Keep the strongest five networks. scanNetworks normally returns RSSI order,
  // but explicitly sorting here keeps behaviour deterministic across cores.
  for (int i = 0; i < found && wifiScanCount < MAX_WIFI_RESULTS; ++i) {
    const String candidate = WiFi.SSID(i);
    if (!candidate.length()) continue;
    bool duplicate = false;
    for (uint8_t j = 0; j < wifiScanCount; ++j) if (wifiScanSsids[j] == candidate) duplicate = true;
    if (duplicate) continue;
    wifiScanSsids[wifiScanCount] = candidate;
    wifiScanRssi[wifiScanCount] = WiFi.RSSI(i);
    ++wifiScanCount;
  }
  WiFi.scanDelete();
  wifiStatusText = String(wifiScanCount) + " networks";
}

void drawWifiKeyboard() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString("WI-FI PASSWORD", 10, 7, 2);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  String ssid = wifiEditSsid;
  if (ssid.length() > 29) ssid = ssid.substring(0, 29);
  canvas.drawString(ssid, 10, 27, 1);
  canvas.drawRoundRect(8, 42, 304, 25, 3, TFT_DARKGREY);
  String masked;
  for (unsigned int i = 0; i < wifiEditPassword.length(); ++i) masked += '*';
  if (masked.length() > 37) masked = masked.substring(masked.length() - 37);
  canvas.drawString(masked, 14, 51, 1);

  const char* rowsUpper[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM_@."};
  const char* rowsLower[] = {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm_@."};
  for (int r = 0; r < 4; ++r) {
    const char* chars = wifiKeyboardShift ? rowsUpper[r] : rowsLower[r];
    for (int c = 0; c < 10; ++c) {
      const int x = c * 32;
      const int y = 76 + r * 28;
      canvas.drawRect(x + 1, y, 30, 25, TFT_DARKGREY);
      char label[2] = {chars[c], 0};
      canvas.setTextDatum(MC_DATUM);
      canvas.setTextColor(TFT_WHITE, TFT_BLACK);
      canvas.drawString(label, x + 16, y + 13, 2);
    }
  }
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString(wifiKeyboardShift ? "SHIFT" : "shift", 35, 201, 1);
  canvas.drawString("SPACE", 102, 201, 1);
  canvas.drawString("BACK", 169, 201, 1);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.drawString("CONNECT", 266, 201, 1);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString(wifiStatusText, 160, 225, 1);
  presentCanvas();
}

void drawSettings() {
  const AquariusSettings& a = settingsManager.values();
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString("ATMOQUARIUM SETTINGS", 10, 8, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.setTextDatum(TR_DATUM);
  const char* titles[] = {"AQUARIUM", "SYSTEM", "WI-FI", "CAL / LOC", "ABOUT"};
  canvas.drawString(titles[settingsPage % 5], 310, 8, 2);
  canvas.drawFastHLine(8, 29, 304, TFT_DARKGREY);

  auto adjustableRow = [&](int y, const String& name, const String& value) {
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(name, 14, y, 2);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString("<", 205, y + 8, 2);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.drawString(value, 258, y + 8, 2);
    canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString(">", 307, y + 8, 2);
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
    adjustableRow(34, "Fish", String(a.fishCount));
    adjustableRow(56, "Bubbles", String(a.bubbleLevel));
    adjustableRow(78, "Plants", String(a.plantLevel));
    adjustableRow(100, "Plant length", String(a.plantLength));
    adjustableRow(122, "Jellyfish", a.jellyfishEnabled ? "ON" : "OFF");
    adjustableRow(144, "Animation", String(a.animationLevel));
    adjustableRow(166, "Weather FX", a.weatherEffects ? "ON" : "OFF");
    adjustableRow(188, "Rare events", String(a.eventFrequency));
  } else if (settingsPage == 1) {
    adjustableRow(34, "Brightness", String(a.brightness));
    adjustableRow(58, "Night bright", String(a.nightBrightness));
    adjustableRow(82, "Auto dim", a.dimMinutes ? String(a.dimMinutes) + " min" : "NEVER");
    adjustableRow(106, "Clock", a.use24Hour ? "24 HOUR" : "12 HOUR");
    adjustableRow(130, "Temperature", a.fahrenheit ? "F" : "C");
    adjustableRow(154, "Sensor poll", String(a.sensorIntervalSec) + " sec");
    canvas.setTextDatum(TL_DATUM); canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("Reset defaults", 14, 182, 2);
    canvas.setTextDatum(MC_DATUM); canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.drawString("RESET", 258, 190, 2);
  } else if (settingsPage == 2) {
    compactInfoRow(38, "Connected", timeService.wifiConnected() ? WiFi.SSID() : "OFFLINE");
    for (uint8_t i = 0; i < wifiScanCount; ++i) {
      String name = wifiScanSsids[i]; if (name.length() > 22) name = name.substring(0,22);
      compactInfoRow(62 + i * 22, String(i + 1) + ". " + name, String(wifiScanRssi[i]) + " dBm");
    }
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.drawRoundRect(18, 178, 126, 24, 3, TFT_DARKGREY);
    canvas.drawString("RESCAN", 81, 190, 2);
    canvas.setTextColor(TFT_ORANGE, TFT_BLACK);
    canvas.drawRoundRect(176, 178, 126, 24, 3, TFT_DARKGREY);
    canvas.drawString("FORGET", 239, 190, 2);
    canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString(wifiStatusText, 160, 207, 1);
  } else if (settingsPage == 3) {
    adjustableRow(40, "Temp offset", String(a.temperatureOffsetC, 1) + " C");
    adjustableRow(68, "Humidity offset", String(a.humidityOffsetPct) + "%");
    adjustableRow(96, "Latitude", String(a.weatherLatitude, 2));
    adjustableRow(124, "Longitude", String(a.weatherLongitude, 2));
    compactInfoRow(158, "Weather location", a.locationName);
    canvas.setTextDatum(MC_DATUM); canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.drawString("RESET CAL / NEWPORT", 160, 190, 2);
  } else {
    String ssid = timeService.wifiConnected() ? WiFi.SSID() : "--";
    if (ssid.length() > 25) ssid = ssid.substring(0, 25);
    compactInfoRow(39,  "Firmware", "v0.1.14");
    compactInfoRow(59,  "Wi-Fi", timeService.wifiConnected() ? "CONNECTED" : "OFFLINE");
    compactInfoRow(79,  "SSID", ssid);
    compactInfoRow(99,  "IP", timeService.wifiConnected() ? WiFi.localIP().toString() : "--");
    compactInfoRow(119, "MAC", WiFi.macAddress());
    compactInfoRow(139, "Sensor", String(indoorSensor.barometricName()));
    compactInfoRow(159, "Location", a.locationName);
    compactInfoRow(179, "Settings", "NVS PERSISTENT");
    canvas.setTextDatum(MC_DATUM); canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
    canvas.drawString("hold title: diagnostics | hold Firmware: demo", 160, 202, 1);
  }

  canvas.drawFastHLine(8, 211, 304, TFT_DARKGREY);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  // Box the Back target so it remains unmistakable on every settings page.
  canvas.drawRoundRect(7, 214, 91, 23, 3, settingsPage == 2 ? TFT_YELLOW : TFT_DARKGREY);
  canvas.drawString("< BACK", 52, 226, 2);
  canvas.drawString("PAGE >", 272, 226, 2);
  canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
  canvas.drawString(settingsPage < 2 ? "tap < or >" : (settingsPage == 2 ? "tap network" : "system information"), 160, 226, 1);
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

  if (settingsPage == 2) {
    if (y >= 176 && y <= 204) {
      if (x < 160) {
        scanWifiNetworks();
        drawSettings();
      } else {
        settingsManager.clearWifiCredentials();
        timeService.setWifiCredentials("", "");
        wifiStatusText = "saved network forgotten";
        // Revert immediately to Secrets.h credentials, if configured.
        timeService.connectSavedWifi(nullptr);
        drawSettings();
      }
      return;
    }
    if (y >= 54 && y < min(174, 54 + (int)wifiScanCount * 22)) {
      const int index = (y - 54) / 22;
      if (index >= 0 && index < wifiScanCount) {
        wifiEditSsid = wifiScanSsids[index];
        wifiEditPassword = "";
        wifiKeyboardOpen = true;
        wifiStatusText = "enter password";
        drawWifiKeyboard();
      }
    }
    return;
  }
  if (settingsPage == 3) {
    const bool decrease=(x>=176&&x<=229), increase=(x>=282&&x<=319);
    const int direction=increase?1:-1;
    if (y>=176 && y<=205) { a.temperatureOffsetC=0.0f; a.humidityOffsetPct=0; a.weatherLatitude=AQUARIUS_LATITUDE; a.weatherLongitude=AQUARIUS_LONGITUDE; a.locationName=AQUARIUS_LOCATION_NAME; }
    else if ((decrease||increase) && y>=30 && y<145) {
      int row=(y-30)/28;
      if(row==0)a.temperatureOffsetC=constrain(a.temperatureOffsetC+direction*0.1f,-10.0f,10.0f);
      if(row==1)a.humidityOffsetPct=constrain((int)a.humidityOffsetPct+direction,-10,10);
      if(row==2){a.weatherLatitude=constrain(a.weatherLatitude+direction*0.10f,-90.0f,90.0f);a.locationName="CUSTOM";}
      if(row==3){a.weatherLongitude=constrain(a.weatherLongitude+direction*0.10f,-180.0f,180.0f);a.locationName="CUSTOM";}
    }
    indoorSensor.setCalibration(a.temperatureOffsetC,a.humidityOffsetPct);
    weatherService.setLocation(a.weatherLatitude,a.weatherLongitude); weatherService.refreshNow(); settingsManager.save(); drawSettings(); return;
  }
  if (settingsPage > 1) return;

  const int row = settingsPage == 0 ? (y - 28) / 22 : (y - 28) / 24;
  const int maxRow = settingsPage == 0 ? 6 : 6;
  if (row < 0 || row > maxRow) return;
  const bool decrease = (x >= 176 && x <= 229);
  const bool increase = (x >= 282 && x <= 319);

  if (settingsPage == 1 && row == 6) {
    if (x >= 190) {
      settingsManager.resetDefaults();
      applyAquariumSettings(); applyBrightness(); drawSettings();
    }
    return;
  }
  if (!decrease && !increase) return;
  const int direction = increase ? 1 : -1;

  if (settingsPage == 0) {
    if (row == 0) a.fishCount = constrain((int)a.fishCount + direction, 1, 10);
    if (row == 1) a.bubbleLevel = constrain((int)a.bubbleLevel + direction, 0, 10);
    if (row == 2) a.plantLevel = constrain((int)a.plantLevel + direction, 0, 10);
    if (row == 3) a.plantLength = constrain((int)a.plantLength + direction, 1, 15);
    if (row == 4) a.jellyfishEnabled = !a.jellyfishEnabled;
    if (row == 5) a.animationLevel = constrain((int)a.animationLevel + direction, 0, 3);
    if (row == 6) a.weatherEffects = !a.weatherEffects;
    if (row == 7) a.eventFrequency = constrain((int)a.eventFrequency + direction, 0, 3);
    applyAquariumSettings();
  } else {
    if (row == 0) { a.brightness = constrain((int)a.brightness + direction * 20, 40, 255); applyBrightness(); }
    if (row == 1) { a.nightBrightness = constrain((int)a.nightBrightness + direction * 20, 20, 220); applyBrightness(); }
    if (row == 2) {
      const uint8_t vals[] = {0, 2, 5, 10, 20, 30, 60};
      int i = discreteIndex(vals, 7, a.dimMinutes); i = constrain(i + direction, 0, 6); a.dimMinutes = vals[i];
    }
    if (row == 3) { a.use24Hour = !a.use24Hour; applyAquariumSettings(); }
    if (row == 4) { a.fahrenheit = !a.fahrenheit; applyAquariumSettings(); }
    if (row == 5) {
      const uint8_t vals[] = {1, 5, 10, 15, 30, 60};
      int i = discreteIndex(vals, 6, a.sensorIntervalSec); i = constrain(i + direction, 0, 5); a.sensorIntervalSec = vals[i];
    }
  }
  settingsManager.save();
  drawSettings();
}

void handleWifiKeyboardTap(int x, int y) {
  if (y >= 76 && y < 188) {
    const int r = (y - 76) / 28;
    const int c = constrain(x / 32, 0, 9);
    const char* rowsUpper[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM_@."};
    const char* rowsLower[] = {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm_@."};
    const char ch = (wifiKeyboardShift ? rowsUpper[r] : rowsLower[r])[c];
    if (wifiEditPassword.length() < 63) wifiEditPassword += ch;
    drawWifiKeyboard(); return;
  }
  if (y >= 188 && y <= 214) {
    if (x < 70) wifiKeyboardShift = !wifiKeyboardShift;
    else if (x < 137) { if (wifiEditPassword.length() < 63) wifiEditPassword += ' '; }
    else if (x < 207) { if (wifiEditPassword.length()) wifiEditPassword.remove(wifiEditPassword.length()-1); }
    else {
      wifiStatusText = "testing credentials..."; drawWifiKeyboard();

      // Never persist unverified credentials. Keep the current known-good
      // configuration so a typo cannot strand Aquarius in NVS.
      const String oldSsid = settingsManager.values().wifiSsid;
      const String oldPass = settingsManager.values().wifiPassword;

      timeService.setWifiCredentials(wifiEditSsid, wifiEditPassword);
      const bool ok = timeService.connectSavedWifi(nullptr);
      if (ok) {
        settingsManager.values().wifiSsid = wifiEditSsid;
        settingsManager.values().wifiPassword = wifiEditPassword;
        settingsManager.save();
        wifiStatusText = "connected + saved";
        wifiKeyboardOpen = false;
        drawSettings();
      } else {
        // Restore the previous saved network (or Secrets.h fallback) and try
        // to get Aquarius back online before returning control to the user.
        timeService.setWifiCredentials(oldSsid, oldPass);
        timeService.connectSavedWifi(nullptr);
        wifiStatusText = "FAILED - not saved";
        drawWifiKeyboard();
      }
      return;
    }
    drawWifiKeyboard();
  }
}

void pollTouch(unsigned long nowMs) {
  const bool touched = touchscreen.touched();
  if (touched && !touchWasDown) {
    TS_Point point = touchscreen.getPoint();
    if (point.z >= AQUARIUS_TOUCH_MIN_PRESSURE) {
      touchDownX = constrain(map(point.x, AQUARIUS_TOUCH_MIN_X, AQUARIUS_TOUCH_MAX_X, 0, AquariusTheme::SCREEN_WIDTH - 1), 0, AquariusTheme::SCREEN_WIDTH - 1);
      touchDownY = constrain(map(point.y, AQUARIUS_TOUCH_MIN_Y, AQUARIUS_TOUCH_MAX_Y, 0, AquariusTheme::SCREEN_HEIGHT - 1), 0, AquariusTheme::SCREEN_HEIGHT - 1);
      touchDownMs = nowMs; lastInteractionMs = nowMs; applyBrightness();
    }
  }

  if (!touched && touchWasDown && touchDownMs != 0) {
    const unsigned long held = nowMs - touchDownMs;
    if (demoOpen) {
      if (held >= 1800UL) { exitDemo(nowMs); }
      else if (touchDownY >= 208) {
        if (touchDownX < 105) setDemoScene((demoScene + DEMO_SCENE_COUNT - 1) % DEMO_SCENE_COUNT, nowMs);
        else if (touchDownX > 215) setDemoScene((demoScene + 1) % DEMO_SCENE_COUNT, nowMs);
        else { demoAuto = !demoAuto; demoSceneStartedMs = nowMs; renderDashboard(nowMs); }
      }
    } else if (diagnosticsOpen) {
      diagnosticsOpen = false; drawSettings();
    } else if (wifiKeyboardOpen) {
      if (touchDownY > 216) { wifiKeyboardOpen = false; drawSettings(); }
      else handleWifiKeyboardTap(touchDownX, touchDownY);
    } else if (settingsOpen) {
      // Hidden tools on About: hold title for diagnostics, hold Firmware row for showcase.
      if (settingsPage == 4 && touchDownY >= 35 && touchDownY < 69 && held >= 4000UL) {
        enterDemo(nowMs);
      } else if (settingsPage == 4 && touchDownY < 35 && held >= 4000UL) {
        diagnosticsOpen = true; drawDiagnostics();
      } else if (touchDownY > 208 && touchDownX < 105) {
        settingsOpen = false; settingsManager.save();
      } else if (touchDownY > 208 && touchDownX > 215) {
        settingsPage = (settingsPage + 1) % 5;
        drawSettings();
      } else changeSettingAt(touchDownX, touchDownY);
    } else if (held >= AQUARIUS_LONG_PRESS_MS) {
      settingsOpen = true; settingsPage = 0; drawSettings();
      Serial.println("Settings          : opened");
    } else if (held >= 40) {
      displayManager.feedFish(nowMs, touchDownX, touchDownY);
      Serial.printf("Aquarium          : feeding triggered at %d,%d\n", touchDownX, touchDownY);
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
  Serial.println(" ATMOQUARIUM v0.1.14");
  Serial.println(" Living Weather Aquarium");
  Serial.println("========================================");

  initialiseDisplay();
  settingsManager.begin();
  indoorSensor.setCalibration(settingsManager.values().temperatureOffsetC, settingsManager.values().humidityOffsetPct);
  weatherService.setLocation(settingsManager.values().weatherLatitude, settingsManager.values().weatherLongitude);
  timeService.setWifiCredentials(settingsManager.values().wifiSsid, settingsManager.values().wifiPassword);
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
  animateBootStatus("AtmoQuarium ready", 550UL);

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

  const unsigned long sensorIntervalMs = (unsigned long)settingsManager.values().sensorIntervalSec * 1000UL;
  if (now - lastSensorUpdateMs >= sensorIntervalMs) {
    lastSensorUpdateMs = now;
    indoorSensor.update();

    if (indoorSensor.reading().pressureValid) {
      pressureTrend.update(indoorSensor.reading().pressureHpa);
    }

    printStatus();
  }

  if (diagnosticsOpen && now - lastDiagnosticsDrawMs >= 1000UL) drawDiagnostics();
  if (demoOpen && demoAuto && now - demoSceneStartedMs >= DEMO_SCENE_DURATION_MS) {
    setDemoScene((demoScene + 1) % DEMO_SCENE_COUNT, now);
  }

  if ((!settingsOpen || demoOpen) && !diagnosticsOpen && now - lastDisplayUpdateMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayUpdateMs = now;
    renderDashboard(now);
  }

  const AquariusSettings& activeSettings = settingsManager.values();
  if (!settingsOpen && activeSettings.dimMinutes > 0 &&
      now - lastInteractionMs > (unsigned long)activeSettings.dimMinutes * 60000UL) {
    if (!backlightDimmed) { Serial.println("DIM: timeout reached"); applyBrightness(true); }
  } else if (backlightDimmed) {
    Serial.println("DIM: wake/timeout cleared"); applyBrightness(false);
  }

  delay(1);
}
