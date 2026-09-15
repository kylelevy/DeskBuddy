"""Exceptions raised by the YETI API client."""


class YetiError(Exception):
    """Base class for all client errors."""


class YetiConnectionError(YetiError):
    """The device could not be reached or returned invalid JSON."""


class YetiAPIError(YetiError):
    """The device rejected a request."""

    def __init__(self, status_code: int, message: str, payload: object = None) -> None:
        self.status_code = status_code
        self.payload = payload
        super().__init__(f"YETI API returned HTTP {status_code}: {message}")
