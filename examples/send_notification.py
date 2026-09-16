"""Send a DeskBuddy notification."""
import argparse
import os

from deskbuddy_client import DeskBuddyClient

parser = argparse.ArgumentParser(description="Send a notification to DeskBuddy")
parser.add_argument("title", nargs="?", default="DeskBuddy notification")
parser.add_argument("body", nargs="?", default="Hello from Python")
parser.add_argument("--icon", default="info")
parser.add_argument("--host", default=os.environ.get("DESKBUDDY_HOST", "deskbuddy.local"))
args = parser.parse_args()
with DeskBuddyClient(args.host) as buddy:
    print(buddy.notify(args.title, args.body, icon=args.icon))
