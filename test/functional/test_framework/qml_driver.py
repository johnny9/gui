#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""QML test automation bridge client."""

import json
import socket
import time


class QmlDriverError(Exception):
    """Raised when the bridge cannot be reached or rejects a command."""


class QmlDriver:
    """Observe and drive the QML application through its local socket."""

    def __init__(self, socket_path, timeout=30):
        self.socket_path = socket_path
        self.timeout = timeout
        self.socket = None
        self.read_buffer = b""
        self._connect()

    def _connect(self):
        deadline = time.monotonic() + self.timeout
        last_error = None
        while time.monotonic() < deadline:
            try:
                self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.socket.settimeout(self.timeout)
                self.socket.connect(self.socket_path)
                return
            except (ConnectionRefusedError, FileNotFoundError, OSError) as error:
                last_error = error
                if self.socket:
                    self.socket.close()
                    self.socket = None
                time.sleep(0.25)
        raise QmlDriverError(f"Could not connect to test bridge at {self.socket_path}: {last_error}")

    def close(self):
        if self.socket:
            self.socket.close()
            self.socket = None

    def get_property(self, object_name, property_name):
        response = self._send({"cmd": "get_property", "objectName": object_name, "prop": property_name})
        self._raise_for_error(response, f"get_property({object_name!r}, {property_name!r})")
        return response["value"]

    def list_objects(self):
        response = self._send({"cmd": "list_objects"})
        self._raise_for_error(response, "list_objects")
        return response["objects"]

    def close_window(self):
        response = self._send({"cmd": "close_window"})
        self._raise_for_error(response, "close_window")

    def _send(self, command):
        if not self.socket:
            raise QmlDriverError("Test bridge is not connected")
        self.socket.sendall((json.dumps(command) + "\n").encode("utf8"))
        return self._receive()

    def _receive(self):
        while b"\n" not in self.read_buffer:
            chunk = self.socket.recv(4096)
            if not chunk:
                raise QmlDriverError("Connection closed by test bridge")
            self.read_buffer += chunk
        line, self.read_buffer = self.read_buffer.split(b"\n", 1)
        return json.loads(line)

    @staticmethod
    def _raise_for_error(response, operation):
        if "error" in response:
            raise QmlDriverError(f"{operation} failed: {response['error']}")
