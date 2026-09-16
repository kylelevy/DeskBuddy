#include "state.h"
#include "config.h"

namespace {
String baseStateName = "cheerful";
String temporaryStateName;
String screenName = "face";
uint32_t temporaryUntilMs = 0;
DeskBuddyState::Notification currentNotification;
bool notificationActive = false;

const char *const ANIMATIONS[] = {"cheerful", "blink", "curious", "happy", "sleepy", "surprised", "celebrate"};
const char *const SCREENS[] = {"face", "clock", "weather", "notification", "wifi", "setup", "error"};

template <size_t N>
bool contains(const char *const (&items)[N], const String &value) {
  for (const char *item : items) {
    if (value == item) return true;
  }
  return false;
}

template <size_t N>
String listJson(const char *const (&items)[N]) {
  String out = "[";
  for (size_t i = 0; i < N; ++i) {
    if (i) out += ',';
    out += '"';
    out += items[i];
    out += '"';
  }
  out += ']';
  return out;
}
}

namespace DeskBuddyState {

void begin() {
  baseStateName = "cheerful";
  temporaryStateName = "";
  screenName = "face";
  temporaryUntilMs = 0;
  notificationActive = false;
}

void setBaseState(const String &name) {
  if (isAnimation(name)) baseStateName = name;
}

void setTemporaryState(const String &name, uint32_t durationMs) {
  if (!isAnimation(name)) return;
  temporaryStateName = name;
  temporaryUntilMs = millis() + constrain(durationMs, 250UL, 3600000UL);
  screenName = "face";
}

void playAnimation(const String &name) {
  setTemporaryState(name, 3000);
}

void showScreen(const String &name, uint32_t durationMs) {
  if (!isScreen(name)) return;
  screenName = name;
  if (name != "face") temporaryUntilMs = millis() + constrain(durationMs, 250UL, 3600000UL);
}

void notify(const Notification &notification) {
  currentNotification = notification;
  currentNotification.title = currentNotification.title.substring(0, DESKBUDDY_TEXT_MAX);
  currentNotification.body = currentNotification.body.substring(0, DESKBUDDY_NOTIFICATION_MAX);
  currentNotification.icon = currentNotification.icon.substring(0, 16);
  if (currentNotification.icon.length() == 0) currentNotification.icon = "info";
  currentNotification.durationMs = constrain(currentNotification.durationMs, 500UL, 60000UL);
  notificationActive = true;
  screenName = "notification";
  temporaryUntilMs = millis() + currentNotification.durationMs;
}

void update(uint32_t now) {
  if (temporaryUntilMs != 0 && (int32_t)(now - temporaryUntilMs) >= 0) {
    temporaryUntilMs = 0;
    temporaryStateName = "";
    notificationActive = false;
    screenName = "face";
  }
}

const String &baseState() { return baseStateName; }
const String &activeState() { return temporaryStateName.length() ? temporaryStateName : baseStateName; }
const String &activeScreen() { return screenName; }
const Notification &notification() { return currentNotification; }
bool hasNotification() { return notificationActive; }
bool temporaryActive(uint32_t now) { return temporaryUntilMs != 0 && (int32_t)(temporaryUntilMs - now) > 0; }
uint32_t temporaryUntil() { return temporaryUntilMs; }
bool isAnimation(const String &name) { return contains(ANIMATIONS, name); }
bool isScreen(const String &name) { return contains(SCREENS, name); }
String animationListJson() { return listJson(ANIMATIONS); }
String screenListJson() { return listJson(SCREENS); }

}  // namespace DeskBuddyState
