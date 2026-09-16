#include "animations.h"
#include "config.h"
#include <FluxGarage_RoboEyes.h>

namespace {
RoboEyes<Adafruit_SSD1306> *face = nullptr;
String selected = "cheerful";
uint32_t lastPlaylistMs = 0;
uint8_t playlistIndex = 0;
const char *const playlist[] = {"cheerful", "curious", "happy", "sleepy", "surprised"};

void apply(const String &name) {
  selected = name;
  face->setDisplayColors(0, 1);
  face->setCyclops(OFF); face->setSweat(OFF); face->setHFlicker(OFF, 0); face->setVFlicker(OFF, 0);
  face->setIdleMode(ON, 2, 3); face->setCuriosity(ON);
  if (name == "happy" || name == "celebrate") {
    face->setWidth(42, 42); face->setHeight(34, 34); face->setBorderradius(12, 12); face->setSpacebetween(8); face->setMood(HAPPY); face->setPosition(DEFAULT); face->anim_laugh();
  } else if (name == "curious") {
    face->setWidth(34, 34); face->setHeight(42, 42); face->setBorderradius(18, 18); face->setSpacebetween(12); face->setMood(DEFAULT); face->setPosition(N); face->blink();
  } else if (name == "sleepy") {
    face->setWidth(46, 46); face->setHeight(24, 24); face->setBorderradius(7, 7); face->setSpacebetween(6); face->setMood(TIRED); face->setPosition(S); face->setIdleMode(OFF);
  } else if (name == "surprised") {
    face->setWidth(32, 32); face->setHeight(44, 44); face->setBorderradius(16, 16); face->setSpacebetween(14); face->setMood(DEFAULT); face->setPosition(DEFAULT); face->setSweat(ON); face->anim_confused();
  } else {
    face->setWidth(40, 40); face->setHeight(34, 34); face->setBorderradius(10, 10); face->setSpacebetween(8); face->setMood(DEFAULT); face->setPosition(DEFAULT);
  }
}
}

namespace DeskBuddyAnimations {
void begin(Adafruit_SSD1306 &display) {
  face = new RoboEyes<Adafruit_SSD1306>(display); face->begin(128, 64, 12); apply("cheerful"); face->close(); face->open();
}
void select(const String &name) { if (face && name != selected) apply(name); }
void update(uint32_t now) {
  if (!face) return;
  if (now - lastPlaylistMs > 18000UL) { lastPlaylistMs = now; playlistIndex = (playlistIndex + 1) % (sizeof(playlist) / sizeof(playlist[0])); select(playlist[playlistIndex]); }
  face->update();
}
const String &current() { return selected; }
}  // namespace DeskBuddyAnimations
