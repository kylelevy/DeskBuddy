#pragma once

#include <Arduino.h>

namespace DeskBuddyState {

struct Notification {
  String title;
  String body;
  String icon;
  uint32_t durationMs = 5000;
};

void begin();
void setBaseState(const String &name);
void setTemporaryState(const String &name, uint32_t durationMs);
void playAnimation(const String &name);
void showScreen(const String &name, uint32_t durationMs);
void notify(const Notification &notification);
void update(uint32_t now);

const String &baseState();
const String &activeState();
const String &activeScreen();
const Notification &notification();
bool hasNotification();
bool temporaryActive(uint32_t now);
uint32_t temporaryUntil();

bool isAnimation(const String &name);
bool isScreen(const String &name);
String animationListJson();
String screenListJson();

}  // namespace DeskBuddyState
