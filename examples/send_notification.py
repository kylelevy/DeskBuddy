"""Send a custom scrolling notification: ``uv run python examples/send_notification.py``."""

import argparse
import os

from yeti_client import YetiClient

parser = argparse.ArgumentParser(description="Send a custom notification to YETI")
parser.add_argument("title", nargs="?", default="YETI notification")
parser.add_argument(
    "body",
    nargs="?",
    default="Hello from the custom notification API over Wi-Fi.",
)
parser.add_argument("--host", default=os.environ.get("YETI_HOST", "yeti.local"))
args = parser.parse_args()

with YetiClient(args.host) as yeti:
    result = yeti.notify(args.title, args.body)

print(f"Notification sent to {args.host} for approximately {result['durationMs']} ms.")
