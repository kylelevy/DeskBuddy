"""Read-only connectivity check suitable for cron or home automation."""

import os
import sys

from yeti_client import YetiClient, YetiConnectionError

host = os.environ.get("YETI_HOST", "yeti.local")
try:
    with YetiClient(host, timeout=3) as yeti:
        status = yeti.status()
except YetiConnectionError as error:
    print(f"YETI unavailable at {host}: {error}", file=sys.stderr)
    raise SystemExit(1) from error

app = status.get("app", {})
wifi = status.get("wifi", {})
print(f"YETI online: {app.get('name', 'YETI')} {app.get('version', '?')}")
print(f"Address: {wifi.get('ip', host)}")
print(f"Mood: {status.get('mood', {}).get('currentMoodLabel', 'unknown')}")
