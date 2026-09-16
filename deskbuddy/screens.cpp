#include "screens.h"
#include "animations.h"
#include "config.h"
#include "network.h"
#include "state.h"
#include "ui_toolkit.h"
#include "weather.h"
#include <WiFi.h>
#include <time.h>

namespace {
Adafruit_SSD1306 *screen = nullptr;
uint32_t lastFlush = 0;
uint32_t lastClockCard = 0;
uint32_t lastWeatherCard = 0;
uint32_t lastFrame = 0;

void drawClock() {
  struct tm timeinfo; if (!getLocalTime(&timeinfo, 20)) { DeskBuddyUI::card("CLOCK", "Waiting for NTP", "Connect to Wi-Fi"); return; }
  char timeText[12]; char dateText[20]; strftime(timeText, sizeof(timeText), "%H:%M", &timeinfo); strftime(dateText, sizeof(dateText), "%a %d %b", &timeinfo);
  DeskBuddyUI::card("CLOCK", timeText, dateText);
}
void drawWeather() {
  if (!DeskBuddyWeather::available()) { DeskBuddyUI::card("WEATHER", "Unavailable", DeskBuddyWeather::error().c_str()); return; }
  DeskBuddyUI::header("WEATHER");
  DeskBuddyUI::icon("weather", 5, 17);
  screen->setTextColor(SSD1306_WHITE);
  screen->setTextSize(2);
  screen->setCursor(25, 15);
  screen->print(DeskBuddyWeather::temperature(), 0);
  screen->print((char)247);
  screen->print("C");
  screen->setTextSize(1);
  screen->setCursor(4, 36);
  screen->print(DeskBuddyWeather::condition());
  screen->setCursor(4, 46);
  screen->print("Rain ");
  screen->print(DeskBuddyWeather::precipitationChance());
  screen->print("% ");
  screen->print(DeskBuddyWeather::precipitationMm(), 1);
  screen->print("mm  Wind ");
  screen->print(DeskBuddyWeather::windSpeedKmh(), 0);
  screen->print("km/h");
  screen->drawLine(0, 55, 127, 55, SSD1306_WHITE);
}
void drawNotification() {
  const auto &note = DeskBuddyState::notification();
  DeskBuddyUI::header(note.title.c_str());
  DeskBuddyUI::notificationIcon(note.icon, 6, 22);
  screen->setCursor(26, 22);
  screen->setTextSize(1);
  screen->setTextColor(SSD1306_WHITE);
  screen->print(note.body.substring(0, 16));
  screen->setCursor(26, 35);
  screen->print(note.body.substring(16, 32));
  screen->drawLine(0, 55, 127, 55, SSD1306_WHITE);
}
void drawStatus(const char *title, const char *line) {
  bool connected = WiFi.status() == WL_CONNECTED;
  const char *status = line;
  if (String(title) == "WI-FI") {
    status = connected ? "Connected" : (DeskBuddyNetwork::setupMode() ? "Pairing mode" : "Connecting...");
  }
  String address = connected ? WiFi.localIP().toString() : (DeskBuddyNetwork::setupMode() ? "AP: 192.168.4.1" : "Waiting for Wi-Fi");
  DeskBuddyUI::card(title, status, address.c_str());
}
}
namespace DeskBuddyScreens {
void begin(Adafruit_SSD1306 &display) { screen = &display; DeskBuddyUI::begin(&display); }
void update(uint32_t now) {
  if (!screen || now - lastFrame < DESKBUDDY_OLED_FRAME_MS) return; lastFrame = now;
  const String &activeScreen = DeskBuddyState::activeScreen();
  // Static screens share the OLED framebuffer, so discard the previous screen
  // before drawing. RoboEyes clears its own frame in drawEyes().
  if (activeScreen != "face") DeskBuddyUI::clear();
  if (activeScreen == "notification") { drawNotification(); screen->display(); return; }
  if (activeScreen == "clock") { drawClock(); screen->display(); return; }
  if (activeScreen == "weather") { drawWeather(); screen->display(); return; }
  if (activeScreen == "wifi") { drawStatus("WI-FI", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"); screen->display(); return; }
  if (activeScreen == "setup") { drawStatus("PAIRING", "Connect to DeskBuddy AP"); screen->display(); return; }
  if (activeScreen == "error") { drawStatus("ERROR", "Check API status"); screen->display(); return; }
  if (now - lastClockCard > DESKBUDDY_CLOCK_INTERVAL_MS) { lastClockCard = now; DeskBuddyState::showScreen("clock", DESKBUDDY_CLOCK_DURATION_MS); return; }
  if (DeskBuddyWeather::available() && now - lastWeatherCard > DESKBUDDY_WEATHER_INTERVAL_MS) { lastWeatherCard = now; DeskBuddyState::showScreen("weather", DESKBUDDY_WEATHER_DURATION_MS); return; }
  DeskBuddyAnimations::select(DeskBuddyState::activeState()); DeskBuddyAnimations::update(now);
}
}  // namespace DeskBuddyScreens
