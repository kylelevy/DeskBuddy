"""Read DeskBuddy status."""
import os

from deskbuddy_client import DeskBuddyClient

with DeskBuddyClient(os.environ.get("DESKBUDDY_HOST", "deskbuddy.local")) as buddy:
    print(buddy.status())
