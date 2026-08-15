# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_executable(bitcoinqml_unit_tests
  test_mainwindow.cpp
  test_unit_tests_main.cpp
)

if(ENABLE_TEST_AUTOMATION)
  target_sources(bitcoinqml_unit_tests PRIVATE test_testbridge.cpp)
endif()

target_link_libraries(bitcoinqml_unit_tests
  PRIVATE
    bitcoinqml
    Qt6::Test
)

get_target_property(qt_qml_test_lib_type Qt6::Core TYPE)
if(qt_qml_test_lib_type STREQUAL "STATIC_LIBRARY")
  target_link_libraries(bitcoinqml_unit_tests PRIVATE Qt6::qtquickcontrols2basicstyleplugin)
endif()

import_plugins(bitcoinqml_unit_tests)

add_test(NAME bitcoinqml_unit_tests
  COMMAND bitcoinqml_unit_tests -platform minimal
)

add_executable(bitcoinqml_qmltests
  qml/qml_tests_main.cpp
)

qt6_add_resources(bitcoinqml_qmltests qml_tests
  PREFIX /test/qml
  FILES qml/tst_mainwindow.qml
)

target_compile_definitions(bitcoinqml_qmltests
  PRIVATE
    QUICK_TEST_SOURCE_DIR=":/test/qml"
)

target_link_libraries(bitcoinqml_qmltests
  PRIVATE
    bitcoinqml
    Qt6::QuickTest
)

if(qt_qml_test_lib_type STREQUAL "STATIC_LIBRARY")
  target_link_libraries(bitcoinqml_qmltests PRIVATE Qt6::qtquickcontrols2basicstyleplugin)
endif()

qt6_import_qml_plugins(bitcoinqml_qmltests)
import_plugins(bitcoinqml_qmltests)

add_test(NAME bitcoinqml_qmltests
  COMMAND bitcoinqml_qmltests -platform minimal
)
