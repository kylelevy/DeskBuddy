# DeskBuddy firmware guide

DeskBuddy is a small Arduino firmware for the ESP32-C3 and a 128×64 I²C OLED. It renders a RoboEyes face, exposes a local JSON API, manages Wi-Fi provisioning, and periodically provides clock and weather screens.

For the detailed implementation model, scheduling diagrams, persistence map, state ownership, and Mermaid figures, see [`firmware-logic.md`](firmware-logic.md).

## Hardware profiles

The primary profile is `DESKBUDDY_BOARD_C3`:

| Resource | Value |
|---|---:|
| SDA | GPIO8 |
| SCL | GPIO9 |
| OLED address | `0x3C` |
| I²C speed | 400 kHz |
| Display | SSD1306 128×64 |

`config.h` also contains a classic ESP32 fallback profile using GPIO21/GPIO22. The profile is selected automatically from the Arduino target, or can be overridden with `DESKBUDDY_BOARD_PROFILE`. The C3 profile is the tested primary target.

## Modules

- `deskbuddy.ino`: boot sequence and cooperative loop
- `config.h`: board, display, timing, and size limits
- `clock.*`: persisted UTC offset and NTP configuration
- `state.*`: runtime state, temporary expiry, screen, and notification
- `animations.*`: RoboEyes presets and automatic playlist
- `ui_toolkit.*`: monochrome drawing primitives and icons
- `screens.*`: application-level OLED screens
- `network.*`: Wi-Fi history, station/AP modes, captive DNS, mDNS, and Web UI
- `weather.*`: Open-Meteo request/cache
- `api.*`: REST route handlers and response generation

## Initialization and loop

Boot initializes Serial, I²C, OLED, RoboEyes, runtime state, weather preferences, clock settings, Wi-Fi history, API routes, and the HTTP server. The OLED shows a rolling Linux-style boot log and is cleared before normal rendering begins.

The main loop is cooperative:

1. Service HTTP and captive DNS.
2. Advance Wi-Fi and NTP startup.
3. Perform weather work when forced or due.
4. Expire temporary display state.
5. Render one frame when the 83 ms display gate allows it.
6. Yield for 1 ms.

The HTTPS weather request is synchronous and can block the loop for up to four seconds. `/api/weather/refresh` only schedules the request; it does not wait for the provider response.

## Rendering

Adafruit GFX and Adafruit SSD1306 are used because FluxGarage RoboEyes accepts that display type directly. The framebuffer is approximately 1 KB. Static screens clear the shared framebuffer before drawing. RoboEyes owns clearing and flushing its face frames.

Automatic playlist entries are:

```text
cheerful, curious, happy, sleepy, surprised
```

The API additionally permits:

```text
blink, celebrate
```

Available static screens are:

```text
face, clock, weather, notification, wifi, setup, error
```

The base state is runtime-only and resets to `cheerful` on reboot. Temporary state, screen, and notification use one shared expiry; a later request replaces the active display rather than entering a queue.

## Wi-Fi

DeskBuddy stores at most three credential pairs in the private `desk_wifi` Preferences namespace. The newest is tried first, followed by older entries after each 20-second timeout. Adding a fourth network promotes it to the front and drops the oldest. Credentials are never returned by the API or UI.

If all saved entries fail, DeskBuddy starts an open `DeskBuddy-Setup-*` access point at `192.168.4.1` with wildcard captive DNS. The setup form posts to `/save`. The response is sent before the AP is disconnected; station reconnection begins shortly afterward.

The AP is open by default because `DESKBUDDY_SETUP_AP_PASSWORD` is empty. The normal local API is unauthenticated. Use DeskBuddy only on a trusted network.

## Clock and weather

The clock stores a UTC offset in the `desk_time` namespace. The range is −12 hours through +14 hours. NTP starts after the first successful Wi-Fi connection using public pools. The clock screen displays local 24-hour time and date, or `Waiting for NTP` until synchronization is available.

Weather stores a `latitude,longitude` string in the `desk_weather` namespace. The Web UI accepts values such as `45.52,-122.67`. Values are validated when refresh runs. Open-Meteo is queried over HTTPS using `setInsecure()`, so certificate verification is disabled. The current summary includes temperature in °C, a friendly WMO condition, precipitation probability/amount, and wind speed. Weather availability is runtime-only and resets after reboot or Wi-Fi loss.

## Build and upload

Install:

- ESP32 Arduino board package
- Adafruit GFX Library
- Adafruit SSD1306
- FluxGarage RoboEyes

Open `deskbuddy/deskbuddy.ino` and select `ESP32C3 Dev Module` for the primary hardware. Recommended settings:

- CPU Frequency: 160 MHz
- Flash Size: match the board, commonly 4MB
- Partition: Default 4MB with spiffs
- Upload Speed: 921600, or 115200 if unreliable
- USB CDC On Boot: Enabled for native-USB boards, otherwise Disabled
- Serial Monitor: 115200 baud

The classic ESP32 profile is available when compiling for a classic ESP32 target. See [`wiring.md`](wiring.md) for connections.

## API and Python client

The route implementation is in `deskbuddy/api.cpp`. The complete request/response contract is in [`python-api.md`](python-api.md). The Python package is `deskbuddy_client`.
