"""A small mood demo. Use only on a YETI you want to annoy politely."""

import os
import time

from yeti_client import YetiClient

with YetiClient(os.environ.get("YETI_HOST", "yeti.local")) as yeti:
    for mood in ("happy", "curious", "smug"):
        result = yeti.set_mood(mood, duration_ms=4000)
        print(f"Set {mood}: {result['mood']['currentMoodLabel']}")
        time.sleep(2)
    yeti.poke()
    print("Poked YETI. Tiny judgment likely.")
