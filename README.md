# DeskBuddy — Version 2.0.0

DeskBuddy Version 2.0.0 is a lightweight ESP32 desktop companion with a 128×64 monochrome OLED, cheerful RoboEyes animations, compact status screens, Wi-Fi provisioning, clock/weather widgets, and a small JSON REST API.

Version 2 replaces the former monolithic V1 sketch with modular firmware, a local Web UI and REST API, and a Python client for automation. See [`CHANGELOG.md`](CHANGELOG.md) for the complete release summary.

The primary target is an ESP32-C3 Super Mini. A classic ESP32 GPIO21/GPIO22 fallback profile is also present in `deskbuddy/config.h`.

## Features

- RoboEyes face with a small automatic animation playlist
- Compiled monochrome UI toolkit and notification icons
- Temporary animation states, screens, and notifications
- Clock screen with configurable UTC offset and NTP synchronization
- Open-Meteo current weather screen with temperature, condition, rain probability/amount, wind, and weather icon
- Private fixed-size history of up to three Wi-Fi networks
- Timeout-based Wi-Fi pairing and captive DNS setup mode
- Minimal black-and-white embedded Web UI
- JSON REST API
- `deskbuddy_client`, a Python abstraction for scripts, services, and webhooks

Removed intentionally: the former mood/personality, memory, sass, sleep, motion, touch, and sequence engines.

## Hardware

### ESP32-C3 primary profile

| OLED | ESP32-C3 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

The default OLED address is `0x3C`. See [`docs/wiring.md`](docs/wiring.md) for the classic ESP32 fallback.

## Arduino dependencies

Install these libraries through the Arduino Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- FluxGarage RoboEyes

Open `deskbuddy/deskbuddy.ino`, select an ESP32-C3 board, and compile. The firmware uses a full 128×64 framebuffer of approximately 1 KB.

### Recommended Arduino IDE settings

| Setting | Value |
|---|---|
| Board | `ESP32C3 Dev Module` |
| USB CDC On Boot | Enabled for native-USB boards; otherwise Disabled |
| Upload Mode | UART0 / Hardware CDC when available |
| CPU Frequency | 160 MHz |
| Flash Mode | QIO, or the board default |
| Flash Size | Match the board, commonly 4MB |
| Partition Scheme | Default 4MB with spiffs |
| Upload Speed | 921600; fall back to 115200 if unreliable |
| Serial Monitor | 115200 baud |

The classic profile is selected automatically when compiling for a classic ESP32 target. Do not select a classic target for the C3 board.

## Boot and runtime

The OLED shows a Linux-style rolling boot log while Serial reports the same phases. After boot, the framebuffer is cleared before normal rendering begins.

The main loop services HTTP/DNS, advances Wi-Fi/NTP, performs due weather work, expires temporary state, and renders an OLED frame approximately every 83 ms. Weather HTTPS requests are synchronous and can block the loop for up to four seconds.

See [`docs/firmware-logic.md`](docs/firmware-logic.md) for detailed ownership, scheduling, persistence, and Mermaid diagrams.

## Wi-Fi behavior

DeskBuddy stores up to three Wi-Fi credential pairs privately in NVS. The newest is attempted first, followed by older entries after each 20-second timeout. Adding a fourth network promotes it to the front and drops the oldest. Credentials are never returned by the API or Web UI.

If all saved networks fail, DeskBuddy starts an open access point named `DeskBuddy-Setup-*` with captive DNS at:

```text
http://192.168.4.1/
```

The AP password is empty by default. The normal page is available at:

```text
http://deskbuddy.local/
```

The local API has no authentication. Keep DeskBuddy on a trusted LAN and do not expose it to the public internet.

## API examples

```bash
curl http://deskbuddy.local/api/health
curl http://deskbuddy.local/api/status
curl http://deskbuddy.local/api/animations
```

Show a temporary animation:

```bash
curl -X POST http://deskbuddy.local/api/state \
  -H 'content-type: application/json' \
  -d '{"name":"happy","duration_ms":8000}'
```

Display a notification with an icon:

```bash
curl -X POST http://deskbuddy.local/api/notification \
  -H 'content-type: application/json' \
  -d '{"title":"Build","body":"Tests passed","icon":"success","duration_ms":5000}'
```

Configure weather and clock:

```bash
curl -X POST http://deskbuddy.local/api/config \
  -H 'content-type: application/json' \
  -d '{"weather_location":"45.52,-122.67","utc_offset_minutes":-420}'

curl -X POST http://deskbuddy.local/api/weather/refresh \
  -H 'content-type: application/json' \
  -d '{}'
```

See [`docs/python-api.md`](docs/python-api.md) for the complete route contract.

## Python client

```bash
python -m pip install -e .
```

```python
from deskbuddy_client import DeskBuddyClient

with DeskBuddyClient("deskbuddy.local") as buddy:
    buddy.notify("Deploy", "Production is healthy", icon="success")
    buddy.set_state("happy", duration_ms=8_000)
    buddy.update_config(utc_offset_minutes=-420)
```

Run checks with:

```bash
.venv/bin/pytest
.venv/bin/ruff check .
```

## Project documentation

- [`docs/firmware-logic.md`](docs/firmware-logic.md): detailed firmware systems and diagrams
- [`docs/firmware-guide.md`](docs/firmware-guide.md): build and architecture guide
- [`docs/python-api.md`](docs/python-api.md): REST and Python API reference
- [`docs/wiring.md`](docs/wiring.md): hardware wiring
- [`examples/`](examples/): Python examples

## License

DeskBuddy is free to use, inspect, modify, and share for noncommercial purposes under the [PolyForm Noncommercial License 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0). Commercial use, including selling the software or products primarily based on it, is not permitted without a separate license from the copyright holder. See [`LICENSE`](LICENSE) for the complete terms.

Because the license restricts commercial use, it is source-available rather than formally Open Source Initiative (OSI)-approved open source. This restriction is intentional so the community can collaborate without the project being commercialized or sold.
