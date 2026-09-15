"""Apply a small, safe configuration change to YETI.

Run with ``YETI_HOST=192.168.1.42 uv run python examples/configure_yeti.py``.
"""

import os

from yeti_client import YetiClient

host = os.environ.get("YETI_HOST", "yeti.local")
with YetiClient(host) as yeti:
    result = yeti.update_config(
        baseMood="deadpan",
        clockEnabled=True,
        clock24h=True,
        sleepEnabled=False,
    )
    print("Configuration saved.")
    print(f"Current mood: {result['status']['mood']['currentMoodLabel']}")
