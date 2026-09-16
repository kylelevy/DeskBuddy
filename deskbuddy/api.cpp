#include "api.h"
#include "clock.h"
#include "config.h"
#include "network.h"
#include "screens.h"
#include "state.h"
#include "weather.h"
#include <Preferences.h>
#include <WiFi.h>

namespace {
String body() { return DeskBuddyNetwork::server().arg("plain"); }
String jsonString(const String &json, const char *key, const String &fallback = "") { String marker = String("\"") + key + "\":\""; int start = json.indexOf(marker); if (start < 0) return fallback; start += marker.length(); int end = json.indexOf('"', start); return end < 0 ? fallback : json.substring(start, end); }
uint32_t jsonUInt(const String &json, const char *key, uint32_t fallback) { String marker = String("\"") + key + "\":"; int start = json.indexOf(marker); if (start < 0) return fallback; start += marker.length(); while (start < (int)json.length() && !isDigit(json[start])) ++start; int end = start; while (end < (int)json.length() && isDigit(json[end])) ++end; String value = json.substring(start, end); return value.length() ? value.toInt() : fallback; }
int16_t jsonInt(const String &json, const char *key, int16_t fallback) { String marker = String("\"") + key + "\":"; int start = json.indexOf(marker); if (start < 0) return fallback; start += marker.length(); int end = start; while (end < (int)json.length() && (isDigit(json[end]) || json[end] == '-')) ++end; String value = json.substring(start, end); return value.length() ? value.toInt() : fallback; }
void reply(const String &payload, int status = 200) { DeskBuddyNetwork::server().send(status, "application/json", payload); }
void handleHealth() { reply("{\"ok\":true,\"name\":\"DeskBuddy\",\"version\":\"" DESKBUDDY_VERSION "\"}"); }
void handleStatus() {
  String out = "{\"ok\":true,\"name\":\"DeskBuddy\",\"version\":\"" DESKBUDDY_VERSION "\",\"board\":\"" DESKBUDDY_BOARD_NAME "\",\"wifi\":{\"connected\":";
  out += WiFi.status() == WL_CONNECTED ? "true" : "false";
  out += ",\"ip\":\"" + DeskBuddyNetwork::ipAddress() + "\",\"rssi\":" + String(DeskBuddyNetwork::rssi());
  out += "},\"state\":{\"base\":\"" + DeskBuddyState::baseState() + "\",\"active\":\"" + DeskBuddyState::activeState() + "\",\"screen\":\"" + DeskBuddyState::activeScreen() + "\"},\"weather\":{\"available\":";
  out += DeskBuddyWeather::available() ? "true" : "false";
  out += ",\"summary\":\"" + DeskBuddyWeather::summary() + "\",\"error\":\"" + DeskBuddyWeather::error() + "\"},\"clock\":{\"utc_offset_minutes\":" + String(DeskBuddyClock::utcOffsetMinutes()) + "},\"display\":{\"frame_rate\":" + String(DeskBuddyScreens::frameRate()) + "}}";
  reply(out);
}
void handleAnimations() { reply("{\"ok\":true,\"animations\":" + DeskBuddyState::animationListJson() + "}"); }
void handleScreens() { reply("{\"ok\":true,\"screens\":" + DeskBuddyState::screenListJson() + "}"); }
void handleNotify() { String json = body(); DeskBuddyState::Notification note; note.title = jsonString(json, "title", "DeskBuddy"); note.body = jsonString(json, "body"); note.icon = jsonString(json, "icon", "info"); note.durationMs = jsonUInt(json, "duration_ms", 5000); if (!note.body.length()) { reply("{\"ok\":false,\"error\":\"body_required\"}", 400); return; } DeskBuddyState::notify(note); reply("{\"ok\":true,\"screen\":\"notification\"}"); }
void handleState() { String json = body(); String name = jsonString(json, "name"); if (!DeskBuddyState::isAnimation(name)) { reply("{\"ok\":false,\"error\":\"unknown_state\"}", 400); return; } DeskBuddyState::setTemporaryState(name, jsonUInt(json, "duration_ms", 8000)); reply("{\"ok\":true}"); }
void handleBase() { String name = jsonString(body(), "name"); if (!DeskBuddyState::isAnimation(name)) { reply("{\"ok\":false,\"error\":\"unknown_state\"}", 400); return; } DeskBuddyState::setBaseState(name); reply("{\"ok\":true}"); }
void handleAnimation() { String name = jsonString(body(), "name"); if (!DeskBuddyState::isAnimation(name)) { reply("{\"ok\":false,\"error\":\"unknown_animation\"}", 400); return; } DeskBuddyState::playAnimation(name); reply("{\"ok\":true}"); }
void handleScreen() { String json = body(); String name = jsonString(json, "name"); if (!DeskBuddyState::isScreen(name)) { reply("{\"ok\":false,\"error\":\"unknown_screen\"}", 400); return; } DeskBuddyState::showScreen(name, jsonUInt(json, "duration_ms", 6000)); reply("{\"ok\":true}"); }
void handleWeather() { DeskBuddyWeather::requestRefresh(); reply("{\"ok\":true,\"status\":\"scheduled\"}"); }
void handleAction() { String name = jsonString(body(), "name"); if (name == "pairing") { DeskBuddyNetwork::enterPairing(); reply("{\"ok\":true}"); } else if (name == "reboot") { reply("{\"ok\":true}"); delay(100); ESP.restart(); } else { reply("{\"ok\":false,\"error\":\"unknown_action\"}", 400); } }
void handleConfig() { String json = body(); String location = jsonString(json, "weather_location"); if (location.length()) { Preferences prefs; prefs.begin("desk_weather", false); prefs.putString("location", location); prefs.end(); DeskBuddyWeather::begin(); } String offset = jsonString(json, "utc_offset_minutes"); if (offset.length()) DeskBuddyClock::setUtcOffsetMinutes(offset.toInt()); else { int16_t numericOffset = jsonInt(json, "utc_offset_minutes", DeskBuddyClock::utcOffsetMinutes()); if (json.indexOf("\"utc_offset_minutes\"") >= 0) DeskBuddyClock::setUtcOffsetMinutes(numericOffset); } if (json.indexOf("\"frame_rate\"") >= 0) { int16_t frameRate = jsonInt(json, "frame_rate", DeskBuddyScreens::frameRate()); DeskBuddyScreens::setFrameRate(constrain(frameRate, DESKBUDDY_MIN_FRAME_RATE, DESKBUDDY_MAX_FRAME_RATE)); } reply("{\"ok\":true}"); }
}
namespace DeskBuddyApi {
void begin() { WebServer &s = DeskBuddyNetwork::server(); s.on("/api/health", HTTP_GET, handleHealth); s.on("/api/status", HTTP_GET, handleStatus); s.on("/api/animations", HTTP_GET, handleAnimations); s.on("/api/screens", HTTP_GET, handleScreens); s.on("/api/notification", HTTP_POST, handleNotify); s.on("/api/state", HTTP_POST, handleState); s.on("/api/state/base", HTTP_POST, handleBase); s.on("/api/animation", HTTP_POST, handleAnimation); s.on("/api/screen", HTTP_POST, handleScreen); s.on("/api/weather/refresh", HTTP_POST, handleWeather); s.on("/api/config", HTTP_POST, handleConfig); s.on("/api/action", HTTP_POST, handleAction); }
}  // namespace DeskBuddyApi
