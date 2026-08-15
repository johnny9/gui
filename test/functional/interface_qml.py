#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the bitcoin-qml interface and automation bridge."""

import platform

from test_framework.qml_driver import QmlDriverError
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal


class QmlInterfaceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0
        self.setup_clean_chain = True

    def setup_network(self):
        pass

    def skip_test_if_missing_module(self):
        self.skip_if_no_qml()
        self.skip_if_no_qml_test_automation()
        if platform.system() == "Windows":
            raise SkipTest("QML test bridge is not supported on Windows")

    def run_test(self):
        harness = None
        try:
            self.log.info("Starting bitcoin-qml and connecting the test bridge")
            harness = self.start_qml()
            gui = harness.driver

            objects = gui.list_objects()
            object_names = {entry["objectName"] for entry in objects}
            assert "mainWindow" in object_names

            self.log.info("Checking the top-level window")
            assert_equal(gui.get_property("mainWindow", "visible"), True)
            assert_equal(gui.get_property("mainWindow", "title"), "Bitcoin Core")

            self.log.info("Checking bridge error handling")
            try:
                gui.get_property("missingObject", "visible")
            except QmlDriverError as error:
                assert "Object not found" in str(error)
            else:
                raise AssertionError("Missing QML object did not return a bridge error")

            self.log.info("Closing the application window through the bridge")
            gui.close_window()
            assert_equal(harness.wait_for_exit(), 0)
        except Exception:
            output = harness.process_output()
            if output:
                self.log.error("bitcoin-qml output:\n%s", output)
            raise
        finally:
            if harness is not None:
                self.stop_qml(harness)


if __name__ == "__main__":
    QmlInterfaceTest(__file__).main()
