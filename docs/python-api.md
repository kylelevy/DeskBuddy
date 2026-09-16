# DeskBuddy Python and REST API

`deskbuddy_client` is a small `httpx` wrapper around the DeskBuddy ESP32 JSON API. It is intended for scripts, background services, webhook handlers, and local automation.

The API has no authentication. Use it only on a trusted LAN. The setup AP is open by default.

## Quick start

```python
from deskbuddy_client import DeskBuddyClient

with DeskBuddyClient("deskbuddy.local") as buddy:
    print(buddy.health())
    buddy.set_state("happy", duration_ms=8_000)
    buddy.notify("Build", "Tests passed", icon="success")
```

Default host: `deskbuddy.local`. A hostname, IP address, or URL may be supplied.

## Client methods

| Python method | HTTP route | Result |
|---|---|---|
| `health()` | `GET /api/health` | Liveness and version |
| `status()` | `GET /api/status` | Wi-Fi, state, weather, and clock status |
| `animations()` | `GET /api/animations` | Supported animation names |
| `screens()` | `GET /api/screens` | Supported screen names |
| `notify(...)` | `POST /api/notification` | Temporary notification |
| `set_state(...)` | `POST /api/state` | Temporary animation state |
| `set_base_state(name)` | `POST /api/state/base` | Runtime-only base animation |
| `play_animation(name)` | `POST /api/animation` | One-shot animation |
| `show_screen(...)` | `POST /api/screen` | Temporary screen |
| `refresh_weather()` | `POST /api/weather/refresh` | Schedules a weather refresh |
| `update_config(...)` | `POST /api/config` | Updates recognized settings |
| `action(name)` | `POST /api/action` | Pairing or reboot |

Successful responses contain `{"ok": true}` plus operation-specific fields. Unknown names and invalid required data return HTTP 400 with `{"ok": false, "error": "..."}`.

## Read routes

### `GET /api/health`

Example response:

```json
{"ok":true,"name":"DeskBuddy","version":"2.0.0"}
```

### `GET /api/status`

The response contains:

```json
{
  "ok": true,
  "name": "DeskBuddy",
  "version": "2.0.0",
  "board": "ESP32-C3 Super Mini",
  "wifi": {
    "connected": true,
    "ip": "10.0.0.234",
    "rssi": -54
  },
  "state": {
    "base": "cheerful",
    "active": "happy",
    "screen": "notification"
  },
  "weather": {
    "available": false,
    "summary": "Weather not configured",
    "error": "Set location"
  },
  "clock": {
    "utc_offset_minutes": -420
  }
}
```

When disconnected, `rssi` is `0` and the IP is the current softAP IP. Saved SSIDs and passwords are never returned.

### `GET /api/animations`

Returns the seven accepted names:

```text
cheerful, blink, curious, happy, sleepy, surprised, celebrate
```

The automatic playlist only cycles `cheerful`, `curious`, `happy`, `sleepy`, and `surprised` every 18 seconds.

### `GET /api/screens`

Returns:

```text
face, clock, weather, notification, wifi, setup, error
```

## Notifications

### `POST /api/notification`

Payload:

```json
{
  "title": "Build",
  "body": "Tests passed",
  "icon": "success",
  "duration_ms": 5000
}
```

- `body` is required.
- `title` defaults to `DeskBuddy`.
- `icon` defaults to `info`.
- `duration_ms` defaults to 5000 and is clamped to 500–60000.
- Title is limited to 64 characters.
- Body is limited to 180 characters in firmware storage; the current renderer shows two 16-character lines.
- Icon names are limited to 16 characters; unknown names use the generic fallback glyph.

Supported icon names:

```text
success, info, warning, error, heart, bell,
mail, github, weather, wifi, rocket, check
```

Python:

```python
buddy.notify(
    "GitHub",
    "A pull request was merged",
    icon="github",
    duration_ms=7_000,
)
```

## States, animations, and screens

### `POST /api/state`

```json
{"name":"happy","duration_ms":8000}
```

The default duration is 8000 ms and is clamped to 250–3600000 ms. The temporary state uses one shared expiry and is not persisted.

### `POST /api/state/base`

```json
{"name":"cheerful"}
```

This changes the base state in RAM only. It resets to `cheerful` after reboot despite the method name.

### `POST /api/animation`

```json
{"name":"celebrate"}
```

The animation is accepted from the compiled list and runs as a short 3000 ms temporary state.

### `POST /api/screen`

```json
{"name":"clock","duration_ms":6000}
```

The default duration is 6000 ms and is clamped to 250–3600000 ms. A `face` request intentionally has no expiry. A later state, screen, or notification replaces the current display request rather than entering a queue.

## Weather and clock configuration

### `POST /api/config`

Recognized fields are:

```json
{
  "weather_location": "45.52,-122.67",
  "utc_offset_minutes": -420
}
```

- `weather_location` is stored in the `desk_weather` Preferences namespace. It is checked when a refresh runs and must contain valid latitude/longitude values.
- `utc_offset_minutes` is persisted in `desk_time` and clamped to −720 through +840.
- A successful weather cache summary includes temperature in °C, a friendly condition, rain probability/amount, and wind speed, for example `18.0°C · Cloudy · Rain 20% · Wind 12 km/h`.
- Unknown fields are silently ignored by the firmware.

### `POST /api/weather/refresh`

Returns immediately:

```json
{"ok":true,"status":"scheduled"}
```

The actual Open-Meteo HTTPS request occurs later in the main loop. It can block the loop for up to four seconds. Weather uses `WiFiClientSecure::setInsecure()` and therefore does not verify the provider certificate.

Python:

```python
buddy.update_config(
    weather_location="45.52,-122.67",
    utc_offset_minutes=-420,
)
buddy.refresh_weather()
```

The clock uses NTP pools after the first successful Wi-Fi connection. Until synchronization completes, the clock screen displays `Waiting for NTP`.

## Maintenance actions

### `POST /api/action`

Supported payloads:

```json
{"name":"pairing"}
```

Immediately enters pairing mode.

```json
{"name":"reboot"}
```

Returns success and restarts after approximately 100 ms.

Other action names return HTTP 400.

## Wi-Fi setup routes

These routes support the captive setup page and are not part of the normal Python control surface:

- `GET /` serves the pairing page in AP mode or the control Web UI when connected.
- `POST /save` accepts form fields `ssid` and `password`.
- `/generate_204` and `/hotspot-detect.html` redirect to the pairing page in AP mode.

Up to three credential pairs are kept privately in NVS. The newest is tried first; adding a fourth discards the oldest. The HTTP response to `/save` is sent before station reconnection begins.

## JSON and error behavior

The firmware uses a small hand-written substring parser rather than a full JSON parser. Send compact, simple JSON with ordinary unescaped strings. Malformed or unsupported requests may return HTTP 400. Clients should treat the response status and `ok` field as authoritative.

## Python errors

All client exceptions derive from `DeskBuddyError`:

- `DeskBuddyConnectionError`: transport failure or invalid JSON response.
- `DeskBuddyAPIError`: HTTP failure or an `ok: false` response. It exposes `status_code` and `payload`.

```python
from deskbuddy_client import DeskBuddyAPIError, DeskBuddyClient

try:
    with DeskBuddyClient(timeout=3) as buddy:
        buddy.play_animation("unknown")
except DeskBuddyAPIError as error:
    print(error.status_code, error.payload)
```

## Testing

The test suite uses `httpx.MockTransport` and does not contact hardware:

```bash
.venv/bin/pytest
.venv/bin/ruff check .
```
