#include "ui_toolkit.h"

namespace {
Adafruit_SSD1306 *canvas = nullptr;

void drawHeart(int16_t x, int16_t y) {
  canvas->fillCircle(x + 3, y + 3, 3, SSD1306_WHITE);
  canvas->fillCircle(x + 9, y + 3, 3, SSD1306_WHITE);
  canvas->fillTriangle(x, y + 4, x + 12, y + 4, x + 6, y + 12, SSD1306_WHITE);
}
void drawCheck(int16_t x, int16_t y) {
  canvas->drawLine(x, y + 6, x + 4, y + 10, SSD1306_WHITE);
  canvas->drawLine(x + 4, y + 10, x + 12, y, SSD1306_WHITE);
}
void drawCross(int16_t x, int16_t y) {
  canvas->drawLine(x, y, x + 11, y + 11, SSD1306_WHITE);
  canvas->drawLine(x + 11, y, x, y + 11, SSD1306_WHITE);
}
}

namespace DeskBuddyUI {
void begin(Adafruit_SSD1306 *display) { canvas = display; }
void clear() { if (canvas) canvas->clearDisplay(); }
void header(const char *title) {
  canvas->setTextColor(SSD1306_WHITE); canvas->setTextSize(1); canvas->setCursor(2, 1); canvas->print(title);
  canvas->drawLine(0, 10, 127, 10, SSD1306_WHITE);
}
void footer(const char *text) {
  canvas->drawLine(0, 55, 127, 55, SSD1306_WHITE); canvas->setCursor(2, 57); canvas->setTextSize(1); canvas->print(text);
}
void centered(const String &text, int16_t y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h; canvas->setTextSize(size); canvas->getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  canvas->setCursor((128 - w) / 2, y); canvas->print(text);
}
void icon(const String &name, int16_t x, int16_t y) { notificationIcon(name, x, y); }
void notificationIcon(const String &name, int16_t x, int16_t y) {
  if (name == "heart") { drawHeart(x, y); return; }
  if (name == "success" || name == "check") { canvas->drawCircle(x + 6, y + 6, 6, SSD1306_WHITE); drawCheck(x + 1, y + 1); return; }
  if (name == "error") { canvas->drawCircle(x + 6, y + 6, 6, SSD1306_WHITE); drawCross(x + 1, y + 1); return; }
  if (name == "warning") { canvas->drawTriangle(x + 6, y, x, y + 12, x + 12, y + 12, SSD1306_WHITE); canvas->drawPixel(x + 6, y + 4, SSD1306_BLACK); canvas->drawPixel(x + 6, y + 5, SSD1306_BLACK); canvas->drawPixel(x + 6, y + 9, SSD1306_BLACK); return; }
  if (name == "wifi") { canvas->drawCircle(x + 6, y + 10, 2, SSD1306_WHITE); canvas->drawLine(x + 1, y + 5, x + 3, y + 7, SSD1306_WHITE); canvas->drawLine(x + 11, y + 5, x + 9, y + 7, SSD1306_WHITE); canvas->drawLine(x + 3, y + 2, x + 5, y + 4, SSD1306_WHITE); canvas->drawLine(x + 9, y + 2, x + 7, y + 4, SSD1306_WHITE); return; }
  if (name == "weather") { canvas->drawCircle(x + 5, y + 7, 4, SSD1306_WHITE); canvas->fillCircle(x + 9, y + 8, 4, SSD1306_WHITE); canvas->fillRect(x + 3, y + 7, 10, 5, SSD1306_WHITE); return; }
  if (name == "bell") { canvas->drawRoundRect(x + 2, y + 2, 9, 9, 3, SSD1306_WHITE); canvas->drawLine(x + 1, y + 11, x + 12, y + 11, SSD1306_WHITE); canvas->drawPixel(x + 6, y + 13, SSD1306_WHITE); return; }
  if (name == "mail") { canvas->drawRect(x, y + 2, 13, 9, SSD1306_WHITE); canvas->drawLine(x, y + 2, x + 6, y + 7, SSD1306_WHITE); canvas->drawLine(x + 12, y + 2, x + 6, y + 7, SSD1306_WHITE); return; }
  if (name == "rocket") { canvas->drawTriangle(x + 6, y, x + 11, y + 9, x + 6, y + 12, SSD1306_WHITE); canvas->drawTriangle(x + 6, y, x + 1, y + 9, x + 6, y + 12, SSD1306_WHITE); canvas->drawCircle(x + 6, y + 5, 1, SSD1306_BLACK); return; }
  if (name == "github") { canvas->drawCircle(x + 6, y + 6, 6, SSD1306_WHITE); canvas->fillCircle(x + 4, y + 5, 1, SSD1306_BLACK); canvas->fillCircle(x + 8, y + 5, 1, SSD1306_BLACK); canvas->drawLine(x + 3, y + 9, x + 9, y + 9, SSD1306_BLACK); return; }
  canvas->drawRect(x + 1, y + 1, 11, 11, SSD1306_WHITE); canvas->drawPixel(x + 6, y + 5, SSD1306_BLACK); canvas->drawPixel(x + 6, y + 8, SSD1306_BLACK);
}
void wifiBadge(int16_t x, int16_t y, int8_t rssi, bool connected) {
  if (!connected) { icon("wifi", x, y); canvas->drawLine(x, y, x + 13, y + 13, SSD1306_WHITE); return; }
  uint8_t bars = rssi > -55 ? 3 : rssi > -70 ? 2 : 1;
  for (uint8_t i = 0; i < 3; ++i) if (i < bars) canvas->fillRect(x + i * 4, y + 10 - i * 3, 3, 2 + i * 3, SSD1306_WHITE);
}
void card(const char *title, const char *line1, const char *line2) {
  canvas->drawRect(0, 0, 128, 64, SSD1306_WHITE); header(title); canvas->setCursor(4, 20); canvas->print(line1);
  if (line2) { canvas->setCursor(4, 34); canvas->print(line2); }
}
}  // namespace DeskBuddyUI
