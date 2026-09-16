"""Cycle through DeskBuddy's compiled animations."""
import os
import time

from deskbuddy_client import DeskBuddyClient

with DeskBuddyClient(os.environ.get("DESKBUDDY_HOST", "deskbuddy.local")) as buddy:
    for animation in ("happy", "curious", "celebrate", "sleepy"):
        print(buddy.play_animation(animation))
        time.sleep(2)
