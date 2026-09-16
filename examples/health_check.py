"""Read-only connectivity check."""
import os
import sys

from deskbuddy_client import DeskBuddyClient, DeskBuddyConnectionError

host = os.environ.get("DESKBUDDY_HOST", "deskbuddy.local")
try:
    with DeskBuddyClient(host, timeout=3) as buddy:
        print(buddy.health())
except DeskBuddyConnectionError as error:
    print(f"DeskBuddy unavailable at {host}: {error}", file=sys.stderr)
    raise SystemExit(1) from error
