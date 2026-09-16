#include "network.h"
#include "config.h"
#include "clock.h"
#include "state.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

namespace {
constexpr uint8_t WIFI_HISTORY_SIZE = 3;
WebServer webServer(80); DNSServer dns; Preferences prefs;
String ssid, password, apName, host = DESKBUDDY_HOSTNAME;
String wifiSsids[WIFI_HISTORY_SIZE], wifiPasswords[WIFI_HISTORY_SIZE];
uint8_t wifiCount = 0, wifiIndex = 0;
bool pairing = false; bool reconnectPending = false;
uint32_t connectStarted = 0, lastRetry = 0, reconnectAt = 0;
void startAp();
void redirect() { webServer.sendHeader("Location", "http://192.168.4.1/", true); webServer.send(302, "text/plain", ""); }
void sendJson(const String &body, int code = 200) { webServer.send(code, "application/json", body); }
void handleSave() { String newSsid = webServer.arg("ssid"); String newPassword = webServer.arg("password"); if (!newSsid.length()) { sendJson("{\"ok\":false,\"error\":\"ssid_required\"}", 400); return; } DeskBuddyNetwork::saveCredentials(newSsid, newPassword); sendJson("{\"ok\":true,\"status\":\"reconnecting\"}"); }
void handleRoot() {
  if (pairing) {
    webServer.send(200, "text/html", "<!doctype html><meta name='viewport' content='width=device-width'><style>body{background:#000;color:#fff;font:16px monospace;max-width:480px;margin:0 auto;padding:1.5rem;box-sizing:border-box}input,button{background:#000;color:#fff;border:1px solid #fff;padding:.7em;margin:.3em 0;width:100%}</style><h1>DeskBuddy // PAIR</h1><form method='post' action='/save'><input name='ssid' placeholder='Wi-Fi name'><input name='password' type='password' placeholder='Password'><button>CONNECT</button></form>");
    return;
  }
  webServer.send(200, "text/html", R"HTML(<!doctype html><meta name='viewport' content='width=device-width'><style>body{background:#000;color:#fff;font:14px monospace;max-width:650px;margin:0 auto;padding:1.5rem;box-sizing:border-box}button,input,select{background:#000;color:#fff;border:1px solid #fff;padding:.55em;margin:.2em}button{cursor:pointer}pre{border:1px solid #fff;padding:1em;white-space:pre-wrap;min-height:4em}.grid{display:flex;flex-wrap:wrap;gap:.2em}section{border:1px solid #fff;padding:.5em;margin:.5em 0}h1{letter-spacing:.1em}</style><h1>DeskBuddy // CONTROL</h1><pre id='s'>loading...</pre><section><b>ANIMATIONS</b><div class='grid'><button onclick="call('/api/animation',{name:'cheerful'})">CHEERFUL</button><button onclick="call('/api/animation',{name:'blink'})">BLINK</button><button onclick="call('/api/animation',{name:'curious'})">CURIOUS</button><button onclick="call('/api/animation',{name:'happy'})">HAPPY</button><button onclick="call('/api/animation',{name:'sleepy'})">SLEEPY</button><button onclick="call('/api/animation',{name:'surprised'})">SURPRISED</button><button onclick="call('/api/animation',{name:'celebrate'})">CELEBRATE</button></div></section><section><b>STATES</b><div class='grid'><button onclick="call('/api/state',{name:'happy',duration_ms:8000})">TEMP HAPPY</button><button onclick="call('/api/state',{name:'curious',duration_ms:8000})">TEMP CURIOUS</button><button onclick="call('/api/state/base',{name:'cheerful'})">BASE CHEERFUL</button></div></section><section><b>SCREENS</b><div class='grid'><button onclick="call('/api/screen',{name:'face',duration_ms:5000})">FACE</button><button onclick="call('/api/screen',{name:'clock',duration_ms:6000})">CLOCK</button><button onclick="call('/api/screen',{name:'weather',duration_ms:6000})">WEATHER</button><button onclick="call('/api/screen',{name:'wifi',duration_ms:6000})">WI-FI</button><button onclick="call('/api/weather/refresh',{})">REFRESH WEATHER</button></div></section><section><b>WEATHER LOCATION</b><br><input id='loc' placeholder='latitude,longitude e.g. 45.52,-122.67'><button onclick="call('/api/config',{weather_location:loc.value})">SAVE LOCATION</button></section><section><b>TIME ZONE</b><br><input id='tz' type='number' min='-720' max='840' step='15' value='0' placeholder='UTC offset minutes, e.g. -420'><button onclick="call('/api/config',{utc_offset_minutes:Number(tz.value)})">SAVE TIME ZONE</button><small>Example: -420 = UTC-7</small></section><section><b>DISPLAY FRAME RATE</b><br><select id='fps'><option value='1'>1 FPS</option><option value='5'>5 FPS</option><option value='10'>10 FPS</option><option value='12'>12 FPS</option><option value='15'>15 FPS</option><option value='20'>20 FPS</option><option value='24'>24 FPS</option><option value='30'>30 FPS</option></select><button onclick="call('/api/config',{frame_rate:Number(fps.value)})">SAVE FRAME RATE</button><small>Controls OLED animation speed (1-30 FPS)</small></section><section><b>NOTIFICATION</b><br><input id='t' placeholder='title'><input id='b' placeholder='message'><select id='i'><option>info</option><option>success</option><option>warning</option><option>error</option><option>heart</option><option>bell</option><option>mail</option><option>github</option><option>weather</option><option>wifi</option><option>rocket</option><option>check</option></select><button onclick='notify()'>SEND</button></section><section><b>MAINTENANCE</b><button onclick="call('/api/action',{name:'pairing'})">PAIRING MODE</button><button onclick="call('/api/action',{name:'reboot'})">REBOOT</button></section><script>async function call(p,x){const r=await fetch(p,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(x)});const j=await r.json();s.textContent=JSON.stringify(j,null,2);setTimeout(load,500)}async function load(){try{const r=await fetch('/api/status');const text=await r.text();try{const data=JSON.parse(text);s.textContent=JSON.stringify(data,null,2);if(data.display&&data.display.frame_rate)fps.value=String(data.display.frame_rate)}catch(e){s.textContent='STATUS RESPONSE INVALID\\n'+text}}catch(e){s.textContent='STATUS REQUEST FAILED\\n'+e}}function notify(){call('/api/notification',{title:t.value,body:b.value,icon:i.value,duration_ms:5000})}load();setInterval(load,5000)</script>)HTML");
}
void handleNotFound() { if (pairing) redirect(); else webServer.send(404, "text/plain", "Not found"); }
void registerRoutes() { webServer.on("/", HTTP_GET, handleRoot); webServer.on("/save", HTTP_POST, handleSave); webServer.on("/generate_204", HTTP_GET, redirect); webServer.on("/hotspot-detect.html", HTTP_GET, redirect); webServer.onNotFound(handleNotFound); }
void startAp() { pairing = true; apName = String("DeskBuddy-Setup-") + String((uint32_t)ESP.getEfuseMac(), HEX).substring(4); WiFi.mode(WIFI_AP_STA); WiFi.softAPConfig(IPAddress(DESKBUDDY_SETUP_IP), IPAddress(DESKBUDDY_SETUP_IP), IPAddress(255,255,255,0)); if (strlen(DESKBUDDY_SETUP_AP_PASSWORD) >= 8) WiFi.softAP(apName.c_str(), DESKBUDDY_SETUP_AP_PASSWORD); else WiFi.softAP(apName.c_str()); dns.start(DESKBUDDY_DNS_PORT, "*", IPAddress(DESKBUDDY_SETUP_IP)); DeskBuddyState::showScreen("setup", 3600000); }
void beginStation() { pairing = false; ssid = wifiSsids[wifiIndex]; password = wifiPasswords[wifiIndex]; Serial.print("[ WiFi ] trying saved network "); Serial.print(wifiIndex + 1); Serial.print("/"); Serial.println(wifiCount); WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.setHostname(host.c_str()); WiFi.begin(ssid.c_str(), password.c_str()); connectStarted = millis(); DeskBuddyState::showScreen("wifi", DESKBUDDY_WIFI_TIMEOUT_MS); }
void loadWifiHistory() {
  wifiCount = 0;
  for (uint8_t i = 0; i < WIFI_HISTORY_SIZE; ++i) {
    wifiSsids[i] = prefs.getString((String("ssid") + i).c_str(), "");
    wifiPasswords[i] = prefs.getString((String("password") + i).c_str(), "");
    if (wifiSsids[i].length()) ++wifiCount;
  }
  if (!wifiCount) {
    String legacySsid = prefs.getString("ssid", "");
    if (legacySsid.length()) {
      wifiSsids[0] = legacySsid;
      wifiPasswords[0] = prefs.getString("password", "");
      wifiCount = 1;
      prefs.putString("ssid0", wifiSsids[0]);
      prefs.putString("password0", wifiPasswords[0]);
    }
  }
  wifiIndex = 0;
}
void persistWifiHistory() {
  for (uint8_t i = 0; i < WIFI_HISTORY_SIZE; ++i) {
    prefs.putString((String("ssid") + i).c_str(), wifiSsids[i]);
    prefs.putString((String("password") + i).c_str(), wifiPasswords[i]);
  }
  prefs.putString("ssid", wifiSsids[0]);
  prefs.putString("password", wifiPasswords[0]);
}
void addWifiCredential(const String &newSsid, const String &newPassword) {
  String oldSsids[WIFI_HISTORY_SIZE];
  String oldPasswords[WIFI_HISTORY_SIZE];
  uint8_t oldCount = wifiCount;
  for (uint8_t i = 0; i < oldCount; ++i) { oldSsids[i] = wifiSsids[i]; oldPasswords[i] = wifiPasswords[i]; }
  wifiSsids[0] = newSsid; wifiPasswords[0] = newPassword;
  wifiCount = 1;
  for (uint8_t i = 0; i < oldCount && wifiCount < WIFI_HISTORY_SIZE; ++i) {
    if (oldSsids[i].length() && oldSsids[i] != newSsid) {
      wifiSsids[wifiCount] = oldSsids[i]; wifiPasswords[wifiCount] = oldPasswords[i]; ++wifiCount;
    }
  }
  wifiIndex = 0;
  persistWifiHistory();
}
}
namespace DeskBuddyNetwork {
WebServer &server() { return webServer; }
void begin() { prefs.begin("desk_wifi", false); host = prefs.getString("host", DESKBUDDY_HOSTNAME); loadWifiHistory(); registerRoutes(); if (wifiCount) beginStation(); else startAp(); }
void startServer() { webServer.begin(); }
void update(uint32_t now) { static bool mdnsStarted = false; static bool ntpStarted = false; if (reconnectPending && (int32_t)(now - reconnectAt) >= 0) { reconnectPending = false; WiFi.softAPdisconnect(true); mdnsStarted = false; beginStation(); return; } if (pairing) return; if (WiFi.status() == WL_CONNECTED) { if (!ntpStarted) { DeskBuddyClock::sync(); ntpStarted = true; } if (!mdnsStarted) { mdnsStarted = MDNS.begin(host.c_str()); DeskBuddyState::showScreen("face", 1000); } return; } if (now - connectStarted > DESKBUDDY_WIFI_TIMEOUT_MS) { if (wifiIndex + 1 < wifiCount) { ++wifiIndex; beginStation(); } else { startAp(); } return; } if (now - lastRetry > DESKBUDDY_WIFI_RETRY_MS) { lastRetry = now; WiFi.reconnect(); } }
void service() { if (pairing) dns.processNextRequest(); webServer.handleClient(); }
bool setupMode() { return pairing; }
const String &setupApName() { return apName; }
const String &hostname() { return host; }
String ipAddress() { return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
int rssi() { return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0; }
void saveCredentials(const String &newSsid, const String &newPassword) { addWifiCredential(newSsid, newPassword); reconnectPending = true; reconnectAt = millis() + 500; }
void enterPairing() { WiFi.disconnect(); startAp(); }
}  // namespace DeskBuddyNetwork
