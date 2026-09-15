"""Demonstrate YETI's memory interactions without changing configuration."""

import os

from yeti_client import YetiClient

host = os.environ.get("YETI_HOST", "yeti.local")
with YetiClient(host) as yeti:
    before = yeti.memory()
    print("Relationship before:", before.get("relationship"))
    result = yeti.memory_action("praise")
    print("Praise response:", result.get("memory", result))
    print("Use memory_action('forgive') or memory_action('annoy') for other interactions.")
