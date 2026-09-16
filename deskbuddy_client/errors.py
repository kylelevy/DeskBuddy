"""Exceptions raised by the DeskBuddy API client."""


class DeskBuddyError(Exception):
    """Base class for DeskBuddy client errors."""


class DeskBuddyConnectionError(DeskBuddyError):
    """The device could not be reached or returned invalid JSON."""


class DeskBuddyAPIError(DeskBuddyError):
    """The device rejected a request."""

    def __init__(self, status_code: int, message: str, payload: object = None) -> None:
        self.status_code = status_code
        self.payload = payload
        super().__init__(f"DeskBuddy API returned HTTP {status_code}: {message}")
