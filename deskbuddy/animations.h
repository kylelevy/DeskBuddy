#pragma once
#include <Adafruit_SSD1306.h>

namespace DeskBuddyAnimations {
void begin(Adafruit_SSD1306 &display);
void select(const String &name);
void update(uint32_t now);
const String &current();
}  // namespace DeskBuddyAnimations
