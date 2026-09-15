"""Read YETI status: ``uv run python examples/read_status.py``."""

import os

from yeti_client import YetiClient

with YetiClient(os.environ.get("YETI_HOST", "yeti.local")) as yeti:
    status = yeti.status()
    print(f"{status['app']['name']} {status['app']['version']}")
    print(f"Mood: {status['mood']['currentMoodLabel']}")
    print(f"Wi-Fi: {status['wifi']['ssid']} ({status['wifi']['ip']})")
    print(f"Uptime: {status['system']['uptimeText']}")
