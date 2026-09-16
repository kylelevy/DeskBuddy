import json

import httpx
import pytest

from deskbuddy_client import DeskBuddyAPIError, DeskBuddyClient


def make_client(handler):
    return DeskBuddyClient(client=httpx.Client(transport=httpx.MockTransport(handler), base_url="http://deskbuddy.local"))

def test_status_and_health():
    def handler(request):
        assert request.url.path in {"/api/status", "/api/health"}
        return httpx.Response(200, json={"ok": True, "name": "DeskBuddy"})
    with make_client(handler) as buddy:
        assert buddy.health()["name"] == "DeskBuddy"
        assert buddy.status()["ok"] is True

def test_notification_payload_includes_icon():
    def handler(request):
        assert request.url.path == "/api/notification"
        assert request.headers["content-type"] == "application/json"
        assert json.loads(request.content) == {
            "title": "Hello",
            "body": "A message",
            "icon": "github",
            "duration_ms": 7000,
        }
        return httpx.Response(200, json={"ok": True})
    with make_client(handler) as buddy:
        assert buddy.notify("Hello", "A message", icon="github", duration_ms=7000)["ok"]

def test_controls_use_json_routes():
    paths = []
    def handler(request):
        paths.append(request.url.path)
        assert request.headers["content-type"] == "application/json"
        return httpx.Response(200, json={"ok": True})
    with make_client(handler) as buddy:
        buddy.set_state("happy", duration_ms=2000)
        buddy.set_base_state("cheerful")
        buddy.play_animation("blink")
        buddy.show_screen("clock")
        buddy.refresh_weather()
        buddy.action("pairing")
    assert paths == [
        "/api/state",
        "/api/state/base",
        "/api/animation",
        "/api/screen",
        "/api/weather/refresh",
        "/api/action",
    ]

def test_api_error_contains_payload():
    def handler(request):
        return httpx.Response(400, json={"ok": False, "error": "unknown_animation"})
    with make_client(handler) as buddy:
        with pytest.raises(DeskBuddyAPIError) as caught:
            buddy.play_animation("nope")
    assert caught.value.status_code == 400
    assert caught.value.payload["error"] == "unknown_animation"

def test_config_payload():
    def handler(request):
        assert json.loads(request.content) == {"weather_location": "45.5,-122.6"}
        return httpx.Response(200, json={"ok": True})
    with make_client(handler) as buddy:
        buddy.update_config(weather_location="45.5,-122.6")
