"""Small value objects used by the YETI client."""

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True, slots=True)
class WifiNetwork:
    """A network returned by the firmware Wi-Fi scanner."""

    ssid: str
    rssi: int
    channel: int
    encryption: str
    bssid: str

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "WifiNetwork":
        return cls(
            ssid=str(value.get("ssid", "")),
            rssi=int(value.get("rssi", 0)),
            channel=int(value.get("channel", 0)),
            encryption=str(value.get("encryption", "")),
            bssid=str(value.get("bssid", "")),
        )


@dataclass(frozen=True, slots=True)
class I2CDevice:
    """A device returned by the firmware I2C scanner."""

    address: str
    decimal: int
    likely: str

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "I2CDevice":
        return cls(
            str(value.get("address", "")),
            int(value.get("decimal", 0)),
            str(value.get("likely", "")),
        )
