"""Send a temporary state to DeskBuddy."""
import os

from deskbuddy_client import DeskBuddyClient

with DeskBuddyClient(os.environ.get("DESKBUDDY_HOST", "deskbuddy.local")) as buddy:
    print(buddy.set_state("happy", duration_ms=5000))
