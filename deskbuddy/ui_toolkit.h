#pragma once

#include <Adafruit_SSD1306.h>

namespace DeskBuddyUI {

void begin(Adafruit_SSD1306 *display);
void clear();
void header(const char *title);
void footer(const char *text);
void centered(const String &text, int16_t y, uint8_t size = 1);
void icon(const String &name, int16_t x, int16_t y);
void wifiBadge(int16_t x, int16_t y, int8_t rssi, bool connected);
void card(const char *title, const char *line1, const char *line2 = nullptr);
void notificationIcon(const String &name, int16_t x, int16_t y);

}  // namespace DeskBuddyUI
