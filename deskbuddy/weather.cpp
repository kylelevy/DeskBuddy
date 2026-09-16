#include "weather.h"
#include "config.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

namespace {
Preferences prefs;
String location = "";
String weatherSummary = "Weather not configured";
String weatherError = "Not configured";
float weatherTemp = 0.0f;
float weatherPrecipitation = 0.0f;
int weatherPrecipitationChance = 0;
float weatherWindSpeed = 0.0f;
String weatherCondition = "Unknown";
bool weatherReady = false;
uint32_t lastAttempt = 0;
bool forceRefresh = false;
String jsonValue(const String &json, const char *key) {
  String marker = String("\"") + key + "\":"; int start = json.indexOf(marker); if (start < 0) return ""; start += marker.length();
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '"')) ++start;
  int end = start; while (end < (int)json.length() && json[end] != ',' && json[end] != '}' && json[end] != '"') ++end;
  return json.substring(start, end);
}
String jsonArrayFirst(const String &json, const char *key) {
  String marker = String("\"") + key + "\":["; int start = json.indexOf(marker); if (start < 0) return ""; start += marker.length();
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '"')) ++start;
  int end = start; while (end < (int)json.length() && json[end] != ',' && json[end] != ']') ++end;
  return json.substring(start, end);
}
String weatherDescription(int code) {
  if (code == 0) return "Clear";
  if (code <= 3) return "Cloudy";
  if (code == 45 || code == 48) return "Foggy";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snow showers";
  if (code >= 95) return "Storm";
  return "Unknown";
}
}
namespace DeskBuddyWeather {
void begin() { prefs.begin("desk_weather", false); location = prefs.getString("location", ""); prefs.end(); }
void requestRefresh() { forceRefresh = true; }
void update(uint32_t now) {
  if (WiFi.status() != WL_CONNECTED || location.length() == 0) { weatherReady = false; weatherError = location.length() ? "Wi-Fi offline" : "Set location"; return; }
  if (!forceRefresh && now - lastAttempt < DESKBUDDY_WEATHER_INTERVAL_MS) return; forceRefresh = false; lastAttempt = now;
  int comma = location.indexOf(','); if (comma <= 0) { weatherError = "Location must be lat,lon"; return; }
  String lat = location.substring(0, comma); lat.trim();
  String lon = location.substring(comma + 1); lon.trim();
  float latitude = lat.toFloat();
  float longitude = lon.toFloat();
  if (isnan(latitude) || isnan(longitude) || latitude < -90.0f || latitude > 90.0f || longitude < -180.0f || longitude > 180.0f) {
    weatherError = "Location must be valid lat,lon";
    return;
  }
  WiFiClientSecure client; client.setInsecure(); client.setTimeout(DESKBUDDY_WEATHER_TIMEOUT_MS);
  HTTPClient http; http.setTimeout(DESKBUDDY_WEATHER_TIMEOUT_MS); http.useHTTP10(true);
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(latitude, 4) + "&longitude=" + String(longitude, 4) + "&current=temperature_2m,weather_code,precipitation,wind_speed_10m&hourly=precipitation_probability&timezone=auto&forecast_days=1";
  Serial.print("[ Weather ] GET ");
  Serial.println(url);
  if (!http.begin(client, url)) { weatherError = "HTTP begin failed"; return; }
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "DeskBuddy/2.0");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    String responseBody = http.getString();
    responseBody.trim();
    weatherError = String("HTTP ") + code;
    Serial.print("[ Weather ] response ");
    Serial.println(code);
    Serial.println(responseBody.substring(0, 180));
    http.end();
    return;
  }
  String json = http.getString();
  http.end();
  int currentStart = json.indexOf("\"current\":");
  String current = currentStart >= 0 ? json.substring(currentStart) : json;
  String temp = jsonValue(current, "temperature_2m");
  String weatherCode = jsonValue(current, "weather_code");
  String precipitation = jsonValue(current, "precipitation");
  String wind = jsonValue(current, "wind_speed_10m");
  String chance = jsonArrayFirst(json, "precipitation_probability");
  if (temp.length() == 0 || weatherCode.length() == 0 || precipitation.length() == 0 || wind.length() == 0) { weatherError = "Invalid weather data"; return; }
  weatherTemp = temp.toFloat();
  weatherPrecipitation = precipitation.toFloat();
  weatherWindSpeed = wind.toFloat();
  weatherPrecipitationChance = chance.length() ? chance.toInt() : 0;
  weatherCondition = weatherDescription(weatherCode.toInt());
  weatherSummary = String(weatherTemp, 1) + "°C · " + weatherCondition + " · Rain " + String(weatherPrecipitationChance) + "% · Wind " + String(weatherWindSpeed, 0) + " km/h";
  weatherReady = true;
  weatherError = "";
}
bool available() { return weatherReady; }
const String &summary() { return weatherSummary; }
const String &error() { return weatherError; }
const String &condition() { return weatherCondition; }
float temperature() { return weatherTemp; }
float precipitationMm() { return weatherPrecipitation; }
int precipitationChance() { return weatherPrecipitationChance; }
float windSpeedKmh() { return weatherWindSpeed; }
}  // namespace DeskBuddyWeather
