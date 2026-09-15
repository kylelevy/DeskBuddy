# Python firmware API

`yeti_client` is a small, typed Python wrapper around the YETI ESP32 firmware's local HTTP API. It hides HTTP details, converts Python values to the form fields expected by the firmware, and exposes firmware operations as methods on `YetiClient`.

The client requires Python 3.10+ and `httpx`:

```bash
uv sync
```

> The firmware API has no authentication. Keep the device on a trusted LAN and do not expose it to the public internet.

## Quick start

```python
from yeti_client import YetiAPIError, YetiClient, YetiConnectionError

try:
    with YetiClient("http://192.168.1.42", timeout=8) as yeti:
        status = yeti.status()
        print(status["wifi"]["ip"])

        yeti.set_mood("happy", duration_ms=10_000)
        yeti.apply_personality("friendly")
        yeti.action("show_clock")
except YetiAPIError as error:
    print(error.status_code, error.payload)
except YetiConnectionError as error:
    print(f"Device unavailable: {error}")
```

The default host is `yeti.local`. A host may be supplied with or without a scheme; `YetiClient("yeti.local")` and `YetiClient("http://yeti.local")` are equivalent.

## `YetiClient`

### Construction and lifecycle

```python
YetiClient(
    host: str = "yeti.local",
    *,
    timeout: float = 5.0,
    client: httpx.Client | None = None,
)
```

- `host` is the device hostname, IP address, or base URL.
- `timeout` is used when the client creates its own `httpx.Client`.
- `client` optionally supplies an existing `httpx.Client` (useful for tests or shared transports).
- Use the client as a context manager, or call `close()` explicitly. An injected HTTP client is not closed by `YetiClient`.

All successful methods return the firmware's decoded JSON object as `dict[str, Any]`, unless noted otherwise. The response is not narrowed to a fixed schema, so newer firmware fields remain available.

## Read and diagnostic methods

| Method | Firmware route | Result |
|---|---|---|
| `status()` | `GET /api/status` | Full device, hardware, Wi-Fi, mood, and configuration status |
| `mood()` | `GET /api/mood` | Current mood/personality state |
| `memory()` | `GET /api/memory` | Memory, grudge, and needs state |
| `scan_wifi()` | `GET /api/scan` | `list[WifiNetwork]` |
| `scan_i2c()` | `GET /api/i2c` | `list[I2CDevice]` |

Wi-Fi scanning can briefly affect the device UI because it uses the ESP32 radio. I²C scanning reports the devices currently visible on the firmware's bus.

```python
with YetiClient() as yeti:
    print(yeti.status())

    for network in yeti.scan_wifi():
        print(network.ssid, network.rssi, network.encryption)

    for device in yeti.scan_i2c():
        print(device.address, device.likely)
```

### Scan models

`scan_wifi()` returns frozen, slot-based `WifiNetwork` objects:

```python
WifiNetwork(
    ssid: str,
    rssi: int,
    channel: int,
    encryption: str,
    bssid: str,
)
```

`scan_i2c()` returns frozen, slot-based `I2CDevice` objects:

```python
I2CDevice(
    address: str,
    decimal: int,
    likely: str,
)
```

## Custom notifications

| Method | Firmware route | Payload |
|---|---|---|
| `notify(title, body)` | `POST /api/notification` | JSON `{ "title": ..., "body": ... }` |

`notify()` displays the title as a static OLED header and scrolls the body using the weather ticker pacing. The firmware derives the display duration from the body width and accepts up to 512 characters for each field.

```python
with YetiClient() as yeti:
    yeti.notify("Build complete", "The firmware test suite passed over Wi-Fi.")
```

## Mood and personality

| Method | Firmware operation |
|---|---|
| `set_mood(mood, duration_ms=8000)` | `POST /api/mood` |
| `set_base_mood(mood)` | `POST /api/mood/base` |
| `random_mood(duration_ms=8000)` | `POST /api/mood/random` |
| `poke()` | `POST /api/mood/poke` |
| `calm()` | `POST /api/mood/calm` |
| `idle_mood_now()` | `POST /api/mood/idle-now` |
| `weather_mood_now()` | `POST /api/mood/weather-now` |
| `wifi_mood_now()` | `POST /api/mood/wifi-now` |
| `movement_mood_now()` | `POST /api/mood/movement-now` |
| `apply_personality(preset)` | `POST /api/personality/preset` |

`set_mood()` sends `mood` and `durationMs`. `random_mood()` sends `durationMs`. `apply_personality()` accepts firmware presets such as `classic`, `friendly`, `sleepy`, `feral`, and `weather`.

```python
with YetiClient() as yeti:
    yeti.set_base_mood("deadpan")
    yeti.set_mood("happy", duration_ms=4_000)
    yeti.random_mood()
    yeti.poke()
```

## Sequences, actions, memory, and sass

### Acting sequences

- `sequence(name)` → `POST /api/sequence` with `sequence=name`
- `stop_sequence()` → `POST /api/sequence/stop`

### General firmware actions

- `action(name)` → `POST /api/action` with `action=name`

Useful action names include `sleep_now`, `sleep_preview`, `wake_now`, `normal`, `angry`, `sleepy`, `happy`, `surprised`, `shocked`, `blink`, `random`, `poke`, `calm`, `demo`, `show_ip`, `weather_refresh`, `sync_clock`, `show_weather`, `show_clock`, `trigger_shake`, and `trigger_touch`. `reboot` is also supported; the device becomes unreachable immediately after the request.

### Memory

- `save_memory(**settings)` → `POST /api/memory/save`; keyword names must use the firmware's field names.
- `memory_action(action)` → `POST /api/memory/{action}`.

Supported memory actions are `save`, `reset`, `forgive`, `annoy`, `praise`, `decay`, `attention`, `calm`, `bore`, `wake`, `rollover`, and `clear-today`. An unknown action raises `ValueError` before any request is sent.

### Sass

- `sass(kind="test")` → `POST /api/sass/{kind}`

Valid kinds are `test`, `judgment`, `grievance`, `random`, and `clear`. An invalid kind raises `ValueError` locally.

## Runtime configuration

`update_config(**settings)` sends `POST /api/config`. Keyword names intentionally match the firmware/WebUI form fields; only send settings that should change:

```python
with YetiClient() as yeti:
    yeti.update_config(
        hostname="yeti-office",
        faceFrameMs=100,
        baseMood="deadpan",
        idleMoodEnabled=True,
        weatherEnabled=False,
        clockEnabled=True,
        clock24h=True,
        clockOffsetMinutes=-420,
        sleepEnabled=True,
        sleepStartTime="21:00",
        sleepEndTime="06:00",
    )
```

Boolean values are encoded as the firmware's expected strings: `True` becomes `"1"` and `False` becomes `"0"`. Other values are converted with `str()`. POST requests use URL-encoded form data, not JSON.

## Errors

All client exceptions derive from `YetiError`:

- `YetiConnectionError`: the device could not be reached, the request failed, or the response was not valid JSON.
- `YetiAPIError`: the firmware returned an HTTP error or a JSON response with `ok: false`. It exposes `status_code` and the original `payload`.

```python
try:
    with YetiClient(timeout=3) as yeti:
        yeti.action("reboot")
except YetiAPIError as error:
    print(error.status_code, error.payload)
except YetiConnectionError:
    print("YETI is unavailable")
```

## Testing without hardware

The test suite uses `httpx.MockTransport`, so it does not contact a physical YETI:

```bash
uv run pytest
uv run ruff check .
```

For a read-only live check, use `status()` or `scan_i2c()` against a device on the local network. Avoid running action, reboot, configuration, or scan operations unintentionally against production hardware.
