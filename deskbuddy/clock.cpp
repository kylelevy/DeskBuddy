#include "clock.h"
#include <Preferences.h>
#include <time.h>

namespace {
Preferences prefs;
int16_t offsetMinutes = 0;
}
namespace DeskBuddyClock {
void begin() {
  prefs.begin("desk_time", false);
  offsetMinutes = prefs.getShort("offset", 0);
  prefs.end();
}
void sync() {
  configTime((long)offsetMinutes * 60L, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
}
void setUtcOffsetMinutes(int16_t minutes) {
  offsetMinutes = constrain(minutes, -720, 840);
  prefs.begin("desk_time", false);
  prefs.putShort("offset", offsetMinutes);
  prefs.end();
  sync();
}
int16_t utcOffsetMinutes() { return offsetMinutes; }
}  // namespace DeskBuddyClock
