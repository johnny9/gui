#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Process harness for QML functional tests."""

import os
from pathlib import Path
import subprocess

from .qml_driver import QmlDriver, QmlDriverError


GUI_STARTUP_TIMEOUT = 30


class QmlTestHarness:
    """Launch bitcoin-qml in an isolated datadir and connect its test bridge."""

    def __init__(self, qml_argv, tmpdir):
        self.qml_argv = list(qml_argv)
        self.tmpdir = Path(tmpdir) / "qml"
        self.datadir = self.tmpdir / "node"
        self.socket_path = self.tmpdir / "test_bridge.sock"
        self.process = None
        self.driver = None

    def start(self, extra_args=None):
        self.datadir.mkdir(parents=True)
        (self.datadir / "bitcoin.conf").write_text(
            "regtest=1\n"
            "[regtest]\n"
            "connect=0\n"
            "discover=0\n"
            "dnsseed=0\n"
            "fixedseeds=0\n"
            "listen=0\n"
            "listenonion=0\n",
            encoding="utf8",
        )

        environment = dict(os.environ)
        environment["QT_QPA_PLATFORM"] = os.getenv("QML_TEST_QPA_PLATFORM", "minimal")
        arguments = self.qml_argv + [
            f"-datadir={self.datadir}",
            f"-test-automation={self.socket_path}",
            "-printtoconsole=1",
        ] + list(extra_args or [])
        self.process = subprocess.Popen(
            arguments,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        try:
            self.driver = QmlDriver(str(self.socket_path), timeout=GUI_STARTUP_TIMEOUT)
        except QmlDriverError as error:
            raise QmlDriverError(f"{error}\n{self.process_output()}") from error

    def wait_for_exit(self, timeout=30):
        return self.process.wait(timeout=timeout)

    def process_output(self):
        if not self.process or self.process.poll() is None:
            return ""
        stdout, stderr = self.process.communicate()
        return "\n".join(
            output.decode("utf8", errors="replace")
            for output in (stdout, stderr)
            if output
        )

    def stop(self):
        if self.driver:
            self.driver.close()
            self.driver = None
        if self.process and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
