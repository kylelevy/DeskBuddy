#pragma once
#include <Adafruit_SSD1306.h>

namespace DeskBuddyScreens {
void begin(Adafruit_SSD1306 &display);
void update(uint32_t now);
}  // namespace DeskBuddyScreens
