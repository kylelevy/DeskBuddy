"""Print Wi-Fi and I2C diagnostics."""

import os

from yeti_client import YetiClient

with YetiClient(os.environ.get("YETI_HOST", "yeti.local")) as yeti:
    print("Nearby Wi-Fi:")
    for network in yeti.scan_wifi():
        print(f"  {network.ssid or '<hidden>'}: {network.rssi} dBm, ch {network.channel}")

    print("I2C devices:")
    for device in yeti.scan_i2c():
        print(f"  {device.address} ({device.decimal}): {device.likely}")
