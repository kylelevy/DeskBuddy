#pragma once
#include <Arduino.h>
namespace DeskBuddyClock {
void begin();
void sync();
void setUtcOffsetMinutes(int16_t minutes);
int16_t utcOffsetMinutes();
}  // namespace DeskBuddyClock
