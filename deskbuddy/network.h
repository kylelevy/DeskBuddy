#pragma once
#include <Arduino.h>
#include <WebServer.h>
namespace DeskBuddyNetwork {
WebServer &server();
void begin();
void startServer();
void update(uint32_t now);
void service();
bool setupMode();
const String &setupApName();
const String &hostname();
String ipAddress();
int rssi();
void saveCredentials(const String &ssid, const String &password);
void enterPairing();
}  // namespace DeskBuddyNetwork
