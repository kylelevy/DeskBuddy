#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "clock.h"
#include "state.h"
#include "animations.h"
#include "screens.h"
#include "network.h"
#include "weather.h"
#include "api.h"

Adafruit_SSD1306 display(DESKBUDDY_WIDTH, DESKBUDDY_HEIGHT, &Wire, -1);
bool displayReady = false;
String bootLines[5];
uint8_t bootLineCount = 0;

void bootStatus(const char *message) {
  Serial.print("[ DeskBuddy ] ");
  Serial.println(message);
  if (!displayReady) return;
  if (bootLineCount < 5) {
    bootLines[bootLineCount++] = message;
  } else {
    for (uint8_t i = 1; i < 5; ++i) bootLines[i - 1] = bootLines[i];
    bootLines[4] = message;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 0);
  display.println("DeskBuddy boot");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  for (uint8_t i = 0; i < bootLineCount; ++i) {
    display.setCursor(2, 12 + i * 10);
    display.println(bootLines[i].substring(0, 20));
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println();
  Serial.println("DeskBuddy " DESKBUDDY_VERSION);
  Serial.println(DESKBUDDY_BOARD_NAME);
  bootStatus("[  OK  ] serial console");
  Wire.begin(DESKBUDDY_SDA, DESKBUDDY_SCL);
  Wire.setClock(DESKBUDDY_I2C_HZ);
  bootStatus("[  OK  ] i2c bus");
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, DESKBUDDY_OLED_ADDRESS);
  if (displayReady) {
    bootStatus("[  OK  ] oled display");
    DeskBuddyAnimations::begin(display);
    bootStatus("[  OK  ] roboeyes face");
    DeskBuddyScreens::begin(display);
    bootStatus("[  OK  ] ui toolkit");
  } else {
    bootStatus("[FAILED] oled display");
  }
  DeskBuddyState::begin();
  bootStatus("[  OK  ] runtime state");
  DeskBuddyWeather::begin();
  bootStatus("[  OK  ] weather cache");
  DeskBuddyClock::begin();
  bootStatus("[  OK  ] clock settings");
  bootStatus("[ .... ] wifi manager");
  DeskBuddyNetwork::begin();
  bootStatus("[  OK  ] wifi manager");
  DeskBuddyApi::begin();
  DeskBuddyNetwork::startServer();
  bootStatus("[  OK  ] http api");
  bootStatus("[  OK  ] deskbuddy ready");
  if (displayReady) {
    display.clearDisplay();
    display.display();
  }
}

void loop() {
  uint32_t now = millis();
  DeskBuddyNetwork::service();
  DeskBuddyNetwork::update(now);
  DeskBuddyWeather::update(now);
  DeskBuddyState::update(now);
  if (displayReady) DeskBuddyScreens::update(now);
  delay(1);
}
