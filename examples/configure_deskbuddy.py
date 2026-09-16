"""Configure DeskBuddy weather coordinates."""
import os

from deskbuddy_client import DeskBuddyClient

host = os.environ.get("DESKBUDDY_HOST", "deskbuddy.local")
with DeskBuddyClient(host) as buddy:
    print(buddy.update_config(weather_location="45.52,-122.67"))
