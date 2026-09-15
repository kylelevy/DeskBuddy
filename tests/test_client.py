import json

import httpx
import pytest

from yeti_client import YetiAPIError, YetiClient
from yeti_client.models import I2CDevice, WifiNetwork


def make_client(handler):
    return YetiClient(client=httpx.Client(transport=httpx.MockTransport(handler), base_url="http://yeti.local"))


def test_status_and_form_encoding():
    def handler(request):
        assert request.url.path == "/api/mood"
        assert request.method == "POST"
        assert request.content == b"mood=happy&durationMs=2500"
        return httpx.Response(200, json={"ok": True, "mood": {"currentMood": "happy"}})

    with make_client(handler) as yeti:
        assert yeti.set_mood("happy", 2500)["mood"]["currentMood"] == "happy"


def test_custom_notification_uses_json_payload():
    def handler(request):
        assert request.url.path == "/api/notification"
        assert request.method == "POST"
        assert request.headers["content-type"] == "application/json"
        assert json.loads(request.content) == {"title": "Hello", "body": "A long message"}
        return httpx.Response(200, json={"ok": True, "durationMs": 2500})

    with make_client(handler) as yeti:
        assert yeti.notify("Hello", "A long message")["durationMs"] == 2500


def test_models_from_device_endpoints():
    def handler(request):
        if request.url.path == "/api/scan":
            return httpx.Response(
                200,
                json=[
                    {
                        "ssid": "lab",
                        "rssi": -42,
                        "channel": 6,
                        "encryption": "WPA2",
                        "bssid": "aa",
                    }
                ],
            )
        return httpx.Response(200, json=[{"address": "0x3c", "decimal": 60, "likely": "OLED"}])

    with make_client(handler) as yeti:
        assert yeti.scan_wifi() == [WifiNetwork("lab", -42, 6, "WPA2", "aa")]
        assert yeti.scan_i2c() == [I2CDevice("0x3c", 60, "OLED")]


def test_api_error_contains_payload():
    def handler(request):
        return httpx.Response(400, json={"ok": False, "error": "unknown_mood"})

    with make_client(handler) as yeti:
        with pytest.raises(YetiAPIError) as caught:
            yeti.set_mood("nope")
    assert caught.value.status_code == 400
    assert caught.value.payload["error"] == "unknown_mood"


def test_boolean_config_fields_use_firmware_values():
    def handler(request):
        assert request.content == b"clockEnabled=1&sleepEnabled=0"
        return httpx.Response(200, json={"ok": True})

    with make_client(handler) as yeti:
        yeti.update_config(clockEnabled=True, sleepEnabled=False)


def test_memory_action_validates_name():
    with make_client(lambda request: httpx.Response(200, json={"ok": True})) as yeti:
        with pytest.raises(ValueError):
            yeti.memory_action("delete-everything")


def test_reaction_and_memory_helpers_use_firmware_routes():
    paths = []

    def handler(request):
        paths.append(request.url.path)
        return httpx.Response(200, json={"ok": True})

    with make_client(handler) as yeti:
        yeti.idle_mood_now()
        yeti.weather_mood_now()
        yeti.wifi_mood_now()
        yeti.movement_mood_now()
        yeti.save_memory(memoryEnabled=True)

    assert paths == [
        "/api/mood/idle-now",
        "/api/mood/weather-now",
        "/api/mood/wifi-now",
        "/api/mood/movement-now",
        "/api/memory/save",
    ]
