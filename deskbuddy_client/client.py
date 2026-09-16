"""Small object-oriented HTTP client for the DeskBuddy firmware API."""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

import httpx

from .errors import DeskBuddyAPIError, DeskBuddyConnectionError


class DeskBuddyClient:
    """Reusable client for a DeskBuddy device on a trusted local network."""

    def __init__(
        self,
        host: str = "deskbuddy.local",
        *,
        timeout: float = 5.0,
        client: httpx.Client | None = None,
    ) -> None:
        base = host if "://" in host else f"http://{host}"
        self.base_url = base.rstrip("/")
        self._owns_client = client is None
        self._http = client or httpx.Client(base_url=self.base_url, timeout=timeout)

    def __enter__(self) -> DeskBuddyClient:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def close(self) -> None:
        if self._owns_client:
            self._http.close()

    def _request(self, method: str, path: str, **kwargs: Any) -> dict[str, Any]:
        try:
            response = self._http.request(method, path, **kwargs)
        except httpx.HTTPError as exc:
            raise DeskBuddyConnectionError(
                f"Could not reach {self.base_url}: {exc}"
            ) from exc
        try:
            payload = response.json()
        except ValueError as exc:
            raise DeskBuddyConnectionError(
                f"DeskBuddy returned non-JSON data for {path}"
            ) from exc
        if response.is_error or (
            isinstance(payload, dict) and payload.get("ok") is False
        ):
            message = (
                payload.get("error", response.text)
                if isinstance(payload, dict)
                else response.text
            )
            raise DeskBuddyAPIError(response.status_code, str(message), payload)
        if not isinstance(payload, dict):
            return {"data": payload}
        return payload

    def _get(self, path: str) -> dict[str, Any]:
        return self._request("GET", path)

    def _post(
        self, path: str, payload: Mapping[str, Any] | None = None
    ) -> dict[str, Any]:
        return self._request("POST", path, json=dict(payload or {}))

    def health(self) -> dict[str, Any]:
        return self._get("/api/health")

    def status(self) -> dict[str, Any]:
        return self._get("/api/status")

    def animations(self) -> dict[str, Any]:
        return self._get("/api/animations")

    def screens(self) -> dict[str, Any]:
        return self._get("/api/screens")

    def notify(
        self,
        title: str,
        body: str,
        *,
        icon: str = "info",
        duration_ms: int = 5000,
    ) -> dict[str, Any]:
        return self._post(
            "/api/notification",
            {"title": title, "body": body, "icon": icon, "duration_ms": duration_ms},
        )

    def set_state(self, name: str, *, duration_ms: int = 8000) -> dict[str, Any]:
        return self._post("/api/state", {"name": name, "duration_ms": duration_ms})

    def set_base_state(self, name: str) -> dict[str, Any]:
        return self._post("/api/state/base", {"name": name})

    def play_animation(self, name: str) -> dict[str, Any]:
        return self._post("/api/animation", {"name": name})

    def show_screen(self, name: str, *, duration_ms: int = 6000) -> dict[str, Any]:
        return self._post("/api/screen", {"name": name, "duration_ms": duration_ms})

    def refresh_weather(self) -> dict[str, Any]:
        return self._post("/api/weather/refresh")

    def update_config(self, **settings: Any) -> dict[str, Any]:
        return self._post("/api/config", settings)

    def action(self, name: str) -> dict[str, Any]:
        return self._post("/api/action", {"name": name})
