#pragma once
#include <Arduino.h>
namespace DeskBuddyWeather {
void begin();
void update(uint32_t now);
void requestRefresh();
bool available();
const String &summary();
const String &error();
const String &condition();
float temperature();
float precipitationMm();
int precipitationChance();
float windSpeedKmh();
}  // namespace DeskBuddyWeather
