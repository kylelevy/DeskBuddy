# Changelog

All notable changes to DeskBuddy are documented here.

## Version 2.0.0

Version 2 is a substantial firmware and tooling refresh. It replaces the monolithic V1 sketch with a modular ESP32 implementation, adds a local control API, and ships a Python client for automation.

### Added

- Modular ESP32 firmware under `deskbuddy/`, split into focused modules for:
  - boot and application lifecycle
  - board configuration and versioning
  - Wi-Fi provisioning and connection management
  - runtime state, temporary states, notifications, and screen selection
  - RoboEyes animations and the OLED UI toolkit
  - clock/NTP synchronization
  - Open-Meteo weather retrieval and caching
  - OLED rendering and display scheduling
  - HTTP/JSON API routes
- ESP32-C3 Super Mini as the primary hardware profile, with a classic ESP32 GPIO21/GPIO22 compatibility profile.
- Linux-style boot status output on Serial and the OLED.
- Automatic animation playlist with supported face animations, one-shot animations, and temporary animation states.
- Clock and weather screens with automatic periodic display cards.
- Wi-Fi credential history for up to three networks, timeout-based retries, captive-DNS setup mode, and mDNS at `deskbuddy.local`.
- Minimal embedded Web UI for status, animations, screens, notifications, weather configuration, time-zone configuration, frame-rate control, pairing, and reboot actions.
- JSON REST endpoints for health, status, animations, screens, notifications, state, weather refresh, configuration, pairing, and reboot.
- Persistent configuration for Wi-Fi credentials, weather location, UTC offset, and OLED frame rate.
- `deskbuddy_client`, a reusable Python/httpx client with context-manager support and typed API/connection error classes.
- Runnable Python examples covering configuration, diagnostics, health checks, interactive control, memory interactions, status, and notifications.
- MockTransport-based Python client tests and Ruff configuration.
- Firmware, API, wiring, and fabrication documentation, including Mermaid architecture/state diagrams.

### Changed

- Firmware reports version `2.0.0` through Serial startup output and the `/api/health` and `/api/status` responses.
- Notifications now support titles, bounded message bodies, configurable durations, and named icons.
- Temporary states, screens, and notifications share a replacement-based expiry model rather than a queue.
- OLED rendering uses a full 128×64 framebuffer and configurable 1–30 FPS display scheduling.
- Weather status includes temperature, WMO-derived condition, precipitation amount and probability, and wind speed.
- Clock configuration uses a persisted UTC offset and NTP synchronization after Wi-Fi connection.
- Wi-Fi setup no longer exposes saved credentials through the API or Web UI.
- The Python package targets Python 3.10+ and uses `httpx` for transport.

### Removed

- The V1 monolithic `yeti_v1_7_4.ino` sketch.
- Legacy mood/personality, memory, sass, sleep, motion, touch, and sequence engines.
- Dependencies on MPU6050/MPU6500 and touch hardware; Version 2 requires only the ESP32 and OLED.

### Compatibility and operational notes

- Version 2 is not wire-compatible with the removed V1 API or firmware architecture.
- The local API has no authentication; run DeskBuddy only on a trusted LAN and do not expose it to the public internet.
- The setup access point is open by default unless `DESKBUDDY_SETUP_AP_PASSWORD` is configured with a password of at least eight characters.
- Weather requests use `WiFiClientSecure::setInsecure()`, so the Open-Meteo provider certificate is not verified.
- The hand-written firmware JSON parser expects compact, simple JSON and does not support general escaped-string handling.

### Validation

- Python client tests use `httpx.MockTransport` and do not require physical hardware; no firmware or hardware integration tests are included in this diff.
- Recommended checks:

  ```bash
  .venv/bin/pytest
  .venv/bin/ruff check .
  ```
