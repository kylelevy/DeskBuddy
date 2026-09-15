# YETI firmware architecture guide

This document explains the firmware in `yeti_v1_7_4/yeti_v1_7_4.ino` for developers who need to understand, debug, or extend the device. It describes the code as it exists today: a single Arduino `.ino` translation unit containing the hardware drivers, behavior engines, HTTP server, embedded WebUI, persistence, and main scheduler.

The short version is:

> YETI is a cooperative, timer-driven state machine. Sensors and network events update state; the mood and memory layers interpret that state; the display layer renders the resulting expression or overlay.

The firmware deliberately avoids an RTOS task architecture and uses `loop()`, `millis()`, and short, bounded operations instead. This keeps the code approachable and prevents the OLED, Wi-Fi server, and personality systems from blocking one another for long periods.

## 1. System purpose and behavior

YETI is an ESP32 desktop companion with four primary responsibilities:

1. **Sense the physical world** through an MPU6050/MPU6500-style accelerometer and, on classic ESP32 boards, capacitive touch.
2. **Maintain connectivity and services** through Wi-Fi, a setup captive portal, mDNS, an HTTP/WebUI server, NTP, and Open-Meteo weather requests.
3. **Maintain an internal behavior model** consisting of moods, personality traits, temporary reactions, scripted sequences, needs, grudges, relationship counters, and lightweight daily memory.
4. **Render a readable face and status information** on a 128×64 SSD1306 OLED.

The personality is not an independent artificial-intelligence service. It is deterministic embedded logic backed by enums, counters, thresholds, cooldowns, weighted choices, and persisted values. The “memory” system is emotional bookkeeping, not a conversation or language model.

## 2. Hardware and platform model

### 2.1 Board profiles

The hardware profile is selected near the top of the sketch. The firmware defines three profiles:

- `YETI_BOARD_ESP32_DEVKIT`: classic ESP32 DevKit/WROOM.
- `YETI_BOARD_ESP32_C3_MINI`: ESP32-C3 Super Mini-style boards; this is the default when the compiler target identifies an ESP32-C3.
- `YETI_BOARD_CUSTOM`: a manually edited profile for another wiring layout.

The current executable C3 profile uses:

| Resource | ESP32-C3 profile |
|---|---:|
| I²C SDA | GPIO8 |
| I²C SCL | GPIO9 |
| OLED address | `0x3C` |
| MPU address | `0x68` |
| Built-in touch | Disabled |

The classic ESP32 profile uses GPIO21/GPIO22 for I²C and enables built-in touch on `T0`/GPIO4. The OLED and MPU share one I²C bus. The OLED reset pin is `-1`, so no separate reset wire is expected.

> When wiring and source disagree, the profile definitions and the `Wire.begin()` call in the `.ino` file are the source of truth. Keep the executable profile and `docs/wiring.md` aligned when changing pins.

The bus is configured with `Wire.setClock(400000)`. The two default device addresses do not conflict, so the OLED and motion sensor can remain connected in parallel.

### 2.2 Hardware libraries and direct access

The sketch uses:

- `Wire` for I²C.
- `Adafruit_SSD1306` and `Adafruit_GFX` for the OLED framebuffer and drawing primitives.
- `FluxGarage_RoboEyes` for the default animated face engine.
- ESP32 `WiFi`, `WebServer`, `DNSServer`, `ESPmDNS`, and `Preferences` APIs.
- `HTTPClient` and `WiFiClientSecure` for weather retrieval.
- `time.h`/`configTime()` for NTP-backed time.

There is no separate MPU library. The MPU is initialized and read directly by writing and reading registers over `Wire`.

### 2.3 Device initialization and failure behavior

Initialization is intentionally best-effort:

- If the OLED fails, `oledReady` is false and the firmware continues without display output.
- If the MPU is absent or cannot be initialized, `mpuReady` is false and motion behavior is disabled.
- Touch is compiled out or disabled by the profile when unsupported.
- If saved Wi-Fi credentials do not connect, the device starts its setup access point and captive DNS portal.

This lets a partially assembled device still expose serial diagnostics and network setup rather than failing as a whole.

## 3. Code organization inside the monolithic sketch

The `.ino` is physically one large file, but its functions form recognizable subsystems. The approximate source landmarks are:

| Area | Representative functions / region |
|---|---|
| Hardware and global configuration | Board profile, face selection, runtime constants, global state near the top |
| General helpers | `jsonEscape`, sanitizers, formatting, event logging |
| Clock and weather | `ensureClockStarted`, `formatClockTime`, `fetchWeatherNow`, `updateWeatherIfNeeded` |
| OLED overlays | `startOledOverlay`, `drawClockSequenceOverlay`, `drawWeatherTickerOverlay`, `drawSassTickerOverlay` |
| Mood and sequences | `setMood`, `applyMood`, `updateMoodEngine`, `startSequence`, `updateSequence` |
| Reactions | `handleYetiEvent`, `updateWifiMoodReactions`, movement reaction helpers |
| Memory and needs | `loadMemorySettings`, `saveMemorySettings`, `updateMemorySystem`, `updateNeeds` |
| Sleep | `enterSleepMode`, `exitSleepMode`, `updateSleepSystem`, preview/burst functions |
| Sensors | `calibrateTouch`, `updateTouch`, `initMPU`, `readAccelG`, `updateMotion` |
| Face rendering | `updateEyeMovement`, `updateBlinking`, classic drawing helpers, `initFaceEngine`, `updateFaceEngine` |
| HTTP/WebUI | embedded HTML, `handleApi*` functions, `setupWebServer` |
| Startup and scheduler | `setup()` and `loop()` at the end |

There are forward declarations for most cross-subsystem functions. This is the main abstraction mechanism used to keep the single translation unit navigable.

## 4. Architectural layers and abstractions

### 4.1 Hardware profile abstraction

Preprocessor constants isolate board-specific hardware details from the rest of the program:

- `YETI_I2C_SDA_PIN`, `YETI_I2C_SCL_PIN`
- `YETI_OLED_ENABLED`, `YETI_OLED_I2C_ADDRESS`, `YETI_OLED_RESET_PIN`
- `YETI_MPU_ENABLED`, `YETI_MPU_I2C_ADDRESS`
- `YETI_TOUCH_ENABLED`, `YETI_TOUCH_PIN`

The behavior code should use readiness flags (`oledReady`, `mpuReady`, `touchReady`) and these profile constants rather than assuming hardware exists.

### 4.2 Domain enums and value objects

The firmware represents behavior with small domain types instead of passing arbitrary strings through the system:

- `Expression`: final face-level expression (`FACE_NORMAL`, `FACE_ANGRY`, `FACE_SLEEPY`, etc.).
- `YetiMood`: higher-level emotional state (`MOOD_HAPPY`, `MOOD_CURIOUS`, `MOOD_WEATHER`, etc.).
- `MoodPriority`: arbitration level from idle to manual.
- `YetiEvent`: reason for a reaction, such as `EVENT_POKED`, `EVENT_WIFI_FAILED`, or `EVENT_GYRO_SHAKEN`.
- `YetiSequence`: non-blocking scripted behavior arc.
- `YetiPhraseCategory`: category for sass ticker text.
- `OledOverlayMode`: status, clock, weather, or sass display mode.

Several structs group related state:

- `YetiPersonality` contains five 0–100 traits.
- `YetiMemoryStats` contains lifetime counters.
- `YetiRelationship` contains affection, annoyance, trust, and suspicion.
- `YetiGrudges` contains event-specific grievance values.
- `YetiNeeds` contains boredom, energy, irritation, and loneliness.
- `YetiDailyMemory` contains today/yesterday counters and the current date key.
- `SensorTriggerConfig` describes whether a sensor trigger is enabled, its hold time, and its expression mask.
- `EventLogEntry` is a fixed-size entry in the 16-entry circular event log.

The split between mood, expression, and event is important: an event says **why** YETI reacted, a mood says **what he feels**, and an expression says **how that feeling is rendered**.

### 4.3 Sanitizers as an input boundary

HTTP form fields and persisted values are not trusted blindly. Helpers such as `sanitizeHostname`, `sanitizeUtcOffsetMinutes`, `sanitizeWeatherUpdateMs`, `sanitizeMinuteOfDay`, `sanitizeSleepAnimMs`, and `clampPercent` constrain values before they enter runtime state.

When adding a setting:

1. Define a safe default and allowed range.
2. Parse it in the WebUI/API handler.
3. Sanitize it before assigning the global.
4. Sanitize again when loading persisted data if appropriate.
5. Persist only the sanitized value.
6. Include it in status/config output so it can be diagnosed.

The JSON parser is intentionally small and local (`jsonFindNumber`, `jsonFindInt`, `jsonFindString`, and related helpers). Most firmware configuration uses URL-encoded HTTP form fields rather than JSON request bodies.

### 4.4 Cooperative timing abstraction

The firmware uses `millis()` timestamps and interval checks instead of long delays. Typical patterns are:

```cpp
if (now - lastRead < interval) return;
lastRead = now;
// perform a bounded update
```

Face pacing, motion sampling, touch sampling, weather refresh, info cards, sleep bursts, reaction cooldowns, memory persistence, and sass all use this pattern. Acting sequences advance one step at a time with `sequenceStepStartedMs` and `nextSequenceStepAtMs`.

The only intentional startup/calibration delays are in initialization paths such as touch calibration and MPU startup. The main loop ends with only a 1 ms idle delay.

## 5. End-to-end runtime flow

### 5.1 `setup()`

Startup follows this order:

1. Start serial logging at 115200 baud.
2. Print firmware and hardware profile information.
3. Seed the random generator and initialize interaction timestamps.
4. Start the configured I²C bus at 400 kHz.
5. Load behavior configuration and memory from `Preferences`/NVS.
6. Initialize the OLED and face engine.
7. Emit the boot event, then show hardware status.
8. Initialize the MPU and calibrate touch when supported.
9. Initialize animation timers for looks, blinks, and expressions.
10. Connect using saved Wi-Fi credentials, or start setup AP/captive DNS if connection fails.
11. Register and start the HTTP server.
12. Show the ready screen and print serial command help.

The order matters: persisted configuration must be loaded before behavior and rendering are initialized; network-dependent clock/weather work is deferred until the loop.

### 5.2 `loop()` scheduling order

Each pass of the loop does the following:

1. Process serial commands.
2. Process captive DNS requests while in setup mode.
3. Let `WebServer` service pending HTTP clients.
4. Start or re-request NTP when appropriate.
5. Refresh weather when its interval is due.
6. Update scheduled sleep state and sleep preview.
7. Sample motion and touch sensors.
8. If sleep does not block rendering, update Wi-Fi reactions and info cards.
9. Update classic eye movement/blinking when the classic renderer is selected.
10. Update demo mode, mood arbitration, idle behavior, sass, and OLED overlays.
11. Run either RoboEyes or the rate-limited classic face renderer.
12. Attempt Wi-Fi reconnects every 10 seconds when not in setup mode.
13. Run memory rollover, grudge decay, needs drift, and throttled persistence.
14. Yield for 1 ms.

The loop is cooperative rather than strictly real-time. A network operation such as a weather request can still be comparatively expensive, so refreshes are infrequent and the rest of the system is interval-gated.

## 6. Hardware-facing subsystems

### 6.1 OLED and face rendering

The OLED uses a 128×64 framebuffer. `Adafruit_SSD1306` owns the buffer and `display.display()` transfers the full buffer over I²C. That transfer is blocking enough that the firmware does not redraw on every loop.

There are two compile-time face engines:

- **RoboEyes (default):** `FluxGarage_RoboEyes` owns animated eye drawing and its own animation pacing. YETI maps its mood/expression state into RoboEyes configuration and calls `updateFaceEngine()`.
- **Classic fallback:** YETI's original hand-drawn renderer draws eyes, mouth, accents, blinks, and look offsets. It is explicitly throttled by `faceFrameMs` (default 83 ms, roughly 12 FPS; allowed range 40–250 ms).

The higher-level code should not need to know which engine is active. It updates mood/expression state, while the face engine translates that state into pixels.

### 6.2 OLED overlays

Overlays temporarily take ownership of the display so readable content is not overwritten by the animated face. The modes are:

- status screen;
- clock time/date sequence;
- scrolling weather ticker;
- sass ticker.

`oledOverlayActive` and `oledOverlayMode` provide the arbitration state. The normal face and sequence logic backs off while an important overlay is active. Overlay content is drawn directly into the SSD1306 framebuffer and flushed at its own cadence.

Sleep is a separate rendering block: it can blank the display with `SSD1306_DISPLAYOFF`, show a periodic hand-drawn sleepy `Zzz` burst, and restore normal rendering on wake.

### 6.3 MPU motion sensing

`initMPU()` checks address `0x68`, reads `WHO_AM_I`, wakes the device through `PWR_MGMT_1`, and configures the sensor registers. `readAccelG()` reads six acceleration bytes and scales them by `8192.0`, matching the configured accelerometer range.

`updateMotion()` samples at most every 25 ms and derives:

- acceleration magnitude and deviation from 1 g;
- frame-to-frame acceleration delta, called jerk;
- tilt delta from the startup baseline;
- a shake score (`max(jerk, magnitude delta)`).

These metrics feed three types of behavior:

- **moved:** ordinary movement reaction;
- **tilted:** pick-up/tilt reaction, latched until the device returns near baseline;
- **shaken:** stronger reaction, legacy sensor override, and sleep wake.

Sensitivity settings convert user-facing percentages into thresholds. Cooldowns prevent a single physical movement from generating a mood on every 25 ms sample.

### 6.4 Capacitive touch

On classic ESP32, `calibrateTouch()` takes 100 baseline samples and sets the touch threshold to 70% of that baseline. `updateTouch()` samples every 30 ms and treats values below the threshold as touched.

A new touch starts a blink and routes through the generic sensor reaction path. A continuing touch extends the configured hold window. On ESP32-C3, the built-in touch path is compiled out because the C3 does not provide the same classic `touchRead()` pads.

## 7. Behavior and personality subsystems

### 7.1 Mood-to-expression pipeline

The behavior path is layered:

```text
input/event
   -> handleYetiEvent() / manual command
   -> mood selection and priority arbitration
   -> setMood() / applyMood()
   -> expressionForMood()
   -> activeExpression()
   -> RoboEyes or classic renderer
```

`baseMood` is the normal fallback. A temporary mood has a deadline (`moodUntilMs`) and priority. Manual actions have the highest priority; alerts and reactions outrank idle behavior. When a temporary mood expires, the mood engine returns toward the base mood.

`Expression` remains as a compatibility/rendering layer. This lets the personality system grow without requiring the renderer to understand every new emotional concept.

### 7.2 Events and automatic reactions

`YetiEvent` is the common input vocabulary for Wi-Fi, weather, WebUI, movement, boot, settings, and user actions. `handleYetiEvent()` is the central reaction dispatcher. It can:

- record memory and daily counters;
- update relationship/grudge values;
- choose a mood;
- start a scripted sequence;
- display a sass phrase;
- enforce reaction cooldowns;
- write a diagnostic event.

Automatic reaction toggles (`weatherMoodEnabled`, `wifiMoodEnabled`, `webMoodEnabled`, and `movementMoodEnabled`) allow the event sources to remain active for diagnostics while their personality consequences are disabled.

### 7.3 Personality presets and idle behavior

The five built-in presets are Classic, Friendly, Sleepy YETI, Feral Goblin, and Weather Gremlin. A preset sets a base mood and five traits:

- grumpiness;
- curiosity;
- sleepiness;
- chaos;
- friendliness.

Idle behavior uses weighted trait values to choose an idle mood and duration. It is separate from manual reactions so random idle changes do not fight a user command or active sequence. `markInteraction()` and idle timers determine when YETI is considered disengaged.

### 7.4 Non-blocking acting sequences

Sequences are small scripted mood arcs such as boot drama, poke reaction, bad-Wi-Fi tantrum, weather reactions, boredom, and settings-saved flourish. `startSequence()` establishes the active sequence; `updateSequence()` advances it based on timestamps.

A sequence never uses a long `delay()`. Each step applies a mood for a bounded duration, waits until the next deadline, and then advances. Manual mood commands can interrupt a sequence. Overlays pause or suppress sequence display effects while keeping the underlying behavior state consistent.

## 8. Memory, grudges, needs, and sass

### 8.1 Persistent memory

Memory is stored in the `yeti_mem` Preferences namespace. It includes:

- lifetime event counters;
- relationship values;
- six grudge values for pokes, shakes, Wi-Fi, weather, reboot, and neglect;
- four needs values for boredom, energy, irritation, and loneliness;
- today/yesterday daily counters and a date key;
- enable flags.

Runtime changes happen in RAM first. `memoryDirty` marks pending persistence, and `saveMemorySettings(false)` will not write more often than once per minute. Explicit user actions can call `saveMemorySettings(true)` for an immediate write. This throttling reduces NVS wear.

### 8.2 Grudge and relationship model

Events call helpers such as `increaseGrudge`, `adjustRelationship`, and `reduceAllGrudges`. Grudges decay every ten minutes, while the mood layer can bias both the selected mood and its duration when a relevant grudge is high. Relationship values affect summaries and personality flavor but are bounded to 0–100.

### 8.3 Needs and daily rollover

`updateNeeds()` runs on a 30-second cadence. Needs drift over time and may trigger a mood only after a longer cooldown. Daily memory rolls today counters into yesterday when the local date changes after clock synchronization. Yesterday's counters can influence the boot sequence and startup attitude.

Without a synchronized clock, the firmware avoids pretending it knows the civil date; manual diagnostic actions can still force a rollover path.

### 8.4 Sass ticker

The sass subsystem chooses short phrases from memory, event, idle, grudge, needs, judgment, and related categories. It applies gates for enabled state, idle-only mode, event/grudge settings, phrase frequency, personality state, overlay conflicts, and cooldowns.

Sass is therefore a presentation consumer of behavior state, not a separate source of truth. It must yield to setup, status, clock, weather, and other readability-critical overlays.

## 9. Network, time, weather, and WebUI

### 9.1 Wi-Fi lifecycle

The firmware loads SSID/password from Preferences and attempts station mode. On failure it starts a setup AP named like `Yeti-Setup-XXXX`, assigns `192.168.4.1`, and runs `DNSServer` so clients are redirected to the setup page. After successful station connection it applies the configured hostname and starts mDNS, normally making `http://yeti.local/` available.

When normal Wi-Fi later drops, the main loop retries every 10 seconds. Wi-Fi state and RSSI also feed the event/mood system.

### 9.2 HTTP server and embedded UI

`WebServer server(80)` serves the embedded dashboard HTML and form/API handlers. The UI is compiled into the sketch, so there is no separate frontend deployment step.

Important routes include:

- `GET /api/status`: combined hardware, system, network, mood, memory, and configuration state.
- `GET /api/scan`: Wi-Fi scan results.
- `GET /api/i2c`: current I²C devices.
- `GET/POST /api/mood`: read or set mood.
- `POST /api/mood/base`, `/api/mood/random`, `/api/mood/poke`, `/api/mood/calm`.
- `POST /api/personality/preset`.
- `GET/POST /api/memory` and `/api/memory/*` actions.
- `POST /api/sass/*` and `/api/sequence*`.
- `POST /api/action`: general commands such as sleep, wake, display cards, weather refresh, and reboot.
- `POST /api/config`: main runtime configuration.
- `POST /forget`: clear Wi-Fi and return to setup.

Most mutating handlers reject requests while setup mode is active, parse URL-encoded arguments, sanitize them, update runtime state, persist configuration, and return JSON containing `ok` plus current status.

The Python client in `yeti_client/` is a convenience wrapper around these same routes; it is not part of the firmware execution model.

### 9.3 Clock and weather

When connected and enabled, `ensureClockStarted()` requests NTP from public servers. Display formatting applies either the manual UTC offset or a validated offset learned from a successful weather response.

Weather uses Open-Meteo's current-weather endpoint with saved latitude/longitude. `fetchWeatherNow()` performs the HTTPS request, extracts only the configured current fields, records success/error state, and emits weather events. `updateWeatherIfNeeded()` schedules refreshes, enforces minimum/maximum intervals, and avoids requests when Wi-Fi or location configuration is unavailable.

Clock and weather are both data sources and display sources: a successful update can affect mood, memory, and sass, while explicit user actions can show a readable OLED card or ticker.

### 9.4 Serial command protocol

The serial console runs at 115200 baud. `handleSerial()` reads one-character commands; newline and carriage-return characters are ignored. The supported commands are:

| Key | Operation |
|---|---|
| `n` | Deadpan mood |
| `a` | Angry mood |
| `s` | Sleepy mood |
| `h` | Happy mood |
| `u` | Curious mood |
| `k` | Startled mood |
| `o` | Smug mood |
| `p` | Poke YETI |
| `x` | Calm / return to base mood |
| `b` | Blink |
| `r` | Random mood |
| `d` | Demo mode |
| `i` | Show network/IP information |
| `t` | Show clock |
| `w` | Refresh/show weather |
| `c` | Calibrate touch on supported classic ESP32 hardware |

Unknown input prints the command help. Serial commands and HTTP actions converge on the same mood, sensor, overlay, and memory functions rather than maintaining a second behavior implementation.

## 10. Sleep subsystem

Sleep is a behavior/display mode, not ESP32 deep sleep. The controller continues running Wi-Fi, the HTTP server, sensor sampling, and the main loop.

The schedule is a start/end minute-of-day window, including overnight windows such as 21:00–06:00. `updateSleepSystem()` decides whether the schedule should own the device. When entering sleep:

- the OLED is normally blanked;
- clock/weather info cards and sass are paused;
- a periodic sleepy `Zzz` animation burst may run;
- motion shake can wake the device.

Manual wake creates a temporary `sleepWakeOverrideUntilMs`, preventing the schedule from immediately reclaiming the display. WebUI actions can start sleep immediately, wake it, or run a finite sleep-animation preview without permanently entering scheduled sleep.

The sleep state is intentionally represented by explicit flags (`sleepModeActive`, `sleepAnimationActive`, `sleepPreviewActive`, and `oledDisplayOff`) rather than inferred solely from the clock. This makes manual controls and diagnostics predictable.

## 11. Persistence and configuration boundaries

The firmware uses ESP32 `Preferences`/NVS for three logical namespaces:

- `wifi`: saved SSID/password used by the station/setup lifecycle.
- `yeti_cfg`: hostname, sensor triggers, face pacing, weather/clock/info-card settings, sleep settings, base mood, personality, automation toggles, and sass settings.
- `yeti_mem`: memory, relationship, grudges, needs, and daily counters.

Configuration loaded from NVS becomes runtime globals. API/WebUI changes update the globals immediately, then call the appropriate save function. A hostname change also reapplies the Wi-Fi hostname and restarts mDNS; the API marks that a reboot may be desirable, but most settings do not require a reboot.

When adding persistent state, decide whether it is:

- a durable user setting;
- a wear-sensitive counter that needs throttling;
- transient runtime state that should never be persisted;
- derived diagnostic state that can be rebuilt at boot.

Do not persist high-frequency animation timestamps or raw sensor samples.

## 12. Diagnostics and troubleshooting path

The firmware has several layers of observability:

- serial logs at 115200 baud;
- a fixed 16-entry event ring buffer;
- `/api/status` JSON;
- `/api/i2c` scan;
- WebUI hardware, performance, memory, mood, and sleep diagnostics;
- sensor metrics such as acceleration magnitude, jerk, tilt delta, and shake score.

A practical debugging order is:

1. Confirm the selected board profile and actual SDA/SCL pins.
2. Confirm the I²C scan sees `0x3C` and `0x68`.
3. Check `oledReady`, `mpuReady`, and touch status in the status API/serial output.
4. Check Wi-Fi state and RSSI before diagnosing clock/weather behavior.
5. Check the event log for the originating event and cooldown/automation state.
6. Check mood priority, active sequence, and overlay/sleep state before changing renderer code.
7. Check memory dirty/save timestamps when a persistent setting appears not to survive reboot.

## 13. Extension guidelines

### Add a new hardware input

1. Add profile-level enable/pin/address constants.
2. Add a readiness flag and initialization function.
3. Add a bounded `update...()` sampler using `millis()`.
4. Convert raw input into a domain event or explicit state, rather than changing the renderer directly.
5. Add status JSON and event-log diagnostics.
6. Disable the feature safely when hardware is absent.

### Add a new mood or reaction

1. Add a `YetiMood` value if it represents a reusable emotional state.
2. Map it in `moodToString`, `stringToMood`, `expressionForMood`, and any status output.
3. Decide its priority and duration.
4. Route triggers through `handleYetiEvent` or a clearly named manual action.
5. Update daily/memory behavior only if the event is meant to be remembered.
6. Verify that overlays, sleep, and active sequences do not produce an unreadable display.

### Add a new WebUI setting

Use the sanitize/assign/save/status path described in section 4.3. Keep the HTTP field name stable because the Python client and external scripts use firmware field names. Return the updated status in the response so the UI can reconcile its form state.

### Preserve responsiveness

Avoid unbounded loops, repeated full OLED flushes, frequent NVS writes, and network calls from the hot path. Prefer timestamp-driven work, explicit cooldowns, and short state transitions. If a feature takes multiple visual or behavioral steps, make it a sequence rather than a blocking function.

## 14. Known design constraints

- The firmware is a large single-file sketch, not a set of independently compiled modules.
- Most state is global and shared by subsystems; enum/struct conventions and named update functions provide the main separation.
- HTTP API access has no authentication. Keep YETI on a trusted LAN and do not expose it directly to the public Internet.
- Weather and NTP depend on network availability and can remain unavailable without preventing local face/sensor behavior.
- The default C3 profile in the firmware currently uses GPIO8/GPIO9 for I²C; verify board-specific silkscreen labels.
- Sleep turns off the display/behavior, but does not power down the ESP32.
- The default RoboEyes renderer is an external library dependency; the classic renderer remains as a compile-time fallback.

## 15. Useful source-reading route

For a quick code tour, read in this order:

1. Board profile and global state near the top of the sketch.
2. The forward declaration block around the first 1,200 lines.
3. `setup()` and `loop()` at the end of the file.
4. `initOLED`, `initMPU`, `updateMotion`, and `updateTouch` for hardware behavior.
5. `setMood`, `handleYetiEvent`, `updateMoodEngine`, and `updateSequence` for behavior arbitration.
6. `loadBehaviorConfig`, `saveBehaviorConfig`, `loadMemorySettings`, and `saveMemorySettings` for persistence.
7. `updateSleepSystem` and overlay functions for display ownership.
8. `setupWebServer` and the `handleApi*` functions for the external control surface.

That path follows the actual dependency direction: hardware and inputs produce events, behavior turns events into state, persistence stores selected state, and renderers/HTTP handlers expose the current result.
