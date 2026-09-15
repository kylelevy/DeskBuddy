"""Object-oriented HTTP client for the YETI firmware API."""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

import httpx

from .errors import YetiAPIError, YetiConnectionError
from .models import I2CDevice, WifiNetwork


class YetiClient:
    """A reusable client for a YETI device on the local network.

    The firmware accepts URL-encoded form fields (not JSON) for POST requests.
    This class handles that wire detail and returns decoded Python dictionaries.
    """

    def __init__(
        self,
        host: str = "yeti.local",
        *,
        timeout: float = 5.0,
        client: httpx.Client | None = None,
    ) -> None:
        base = host if "://" in host else f"http://{host}"
        self.base_url = base.rstrip("/")
        self._owns_client = client is None
        self._http = client or httpx.Client(base_url=self.base_url, timeout=timeout)

    def __enter__(self) -> YetiClient:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def close(self) -> None:
        """Close the underlying HTTP connection pool."""
        if self._owns_client:
            self._http.close()

    def _request(self, method: str, path: str, **kwargs: Any) -> Any:
        try:
            response = self._http.request(method, path, **kwargs)
        except httpx.HTTPError as exc:
            raise YetiConnectionError(f"Could not reach {self.base_url}: {exc}") from exc
        try:
            payload = response.json()
        except ValueError as exc:
            raise YetiConnectionError(f"YETI returned non-JSON data for {path}") from exc
        if response.is_error:
            message = (
                payload.get("error", response.text)
                if isinstance(payload, dict)
                else response.text
            )
            raise YetiAPIError(response.status_code, str(message), payload)
        if isinstance(payload, dict) and payload.get("ok") is False:
            raise YetiAPIError(
                response.status_code,
                str(payload.get("error", "request failed")),
                payload,
            )
        return payload

    def _get(self, path: str) -> Any:
        return self._request("GET", path)

    def _post(self, path: str, fields: Mapping[str, Any] | None = None) -> Any:
        return self._request("POST", path, data=_form_fields(fields or {}))

    def status(self) -> dict[str, Any]:
        """Return the full device, hardware, mood, and system status."""
        return self._get("/api/status")

    def scan_wifi(self) -> list[WifiNetwork]:
        """Scan for nearby Wi-Fi networks (can briefly affect the device UI)."""
        return [WifiNetwork.from_dict(item) for item in self._get("/api/scan")]

    def scan_i2c(self) -> list[I2CDevice]:
        """Scan the device I2C bus."""
        return [I2CDevice.from_dict(item) for item in self._get("/api/i2c")]

    def mood(self) -> dict[str, Any]:
        return self._get("/api/mood")

    def set_mood(self, mood: str, duration_ms: int = 8000) -> dict[str, Any]:
        return self._post("/api/mood", {"mood": mood, "durationMs": duration_ms})

    def set_base_mood(self, mood: str) -> dict[str, Any]:
        return self._post("/api/mood/base", {"mood": mood})

    def random_mood(self, duration_ms: int = 8000) -> dict[str, Any]:
        return self._post("/api/mood/random", {"durationMs": duration_ms})

    def poke(self) -> dict[str, Any]:
        """Ask YETI to react as if it was poked."""
        return self._post("/api/mood/poke")

    def calm(self) -> dict[str, Any]:
        """Trigger YETI's calming interaction."""
        return self._post("/api/mood/calm")

    def idle_mood_now(self) -> dict[str, Any]:
        """Run the configured idle behavior immediately."""
        return self._post("/api/mood/idle-now")

    def weather_mood_now(self) -> dict[str, Any]:
        """Run the configured weather reaction immediately."""
        return self._post("/api/mood/weather-now")

    def wifi_mood_now(self) -> dict[str, Any]:
        """Run the configured Wi-Fi reaction immediately."""
        return self._post("/api/mood/wifi-now")

    def movement_mood_now(self) -> dict[str, Any]:
        """Run the configured movement reaction immediately."""
        return self._post("/api/mood/movement-now")

    def apply_personality(self, preset: str) -> dict[str, Any]:
        return self._post("/api/personality/preset", {"preset": preset})

    def sequence(self, name: str) -> dict[str, Any]:
        return self._post("/api/sequence", {"sequence": name})

    def stop_sequence(self) -> dict[str, Any]:
        return self._post("/api/sequence/stop")

    def memory(self) -> dict[str, Any]:
        return self._get("/api/memory")

    def save_memory(self, **settings: Any) -> dict[str, Any]:
        """Persist memory and sass settings, using firmware field names."""
        return self._post("/api/memory/save", settings)

    def memory_action(self, action: str) -> dict[str, Any]:
        """Run a memory action such as ``praise``, ``annoy``, or ``forgive``."""
        allowed = {
            "save",
            "reset",
            "forgive",
            "annoy",
            "praise",
            "decay",
            "attention",
            "calm",
            "bore",
            "wake",
            "rollover",
            "clear-today",
        }
        if action not in allowed:
            raise ValueError(f"Unknown memory action {action!r}; choose from {sorted(allowed)}")
        return self._post(f"/api/memory/{action}")

    def sass(self, kind: str = "test") -> dict[str, Any]:
        """Trigger a sass endpoint: test, judgment, grievance, random, or clear."""
        if kind not in {"test", "judgment", "grievance", "random", "clear"}:
            raise ValueError("kind must be test, judgment, grievance, random, or clear")
        return self._post(f"/api/sass/{kind}")

    def action(self, name: str) -> dict[str, Any]:
        """Run a firmware action, e.g. ``sleep_now``, ``wake_now``, or ``reboot``."""
        return self._post("/api/action", {"action": name})

    def update_config(self, **settings: Any) -> dict[str, Any]:
        """Save runtime settings using the firmware's form field names.

        Boolean values are converted to the firmware's expected ``1``/``0``.
        See README.md for supported fields.
        """
        return self._post("/api/config", settings)


def _form_fields(fields: Mapping[str, Any]) -> dict[str, str]:
    return {
        key: ("1" if value else "0") if isinstance(value, bool) else str(value)
        for key, value in fields.items()
    }
