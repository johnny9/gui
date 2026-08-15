# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(ImportQtPlugins)

set(CMAKE_AUTOMOC ON)

qt6_add_resources(BITCOIN_QML_QRC bitcoin_qml.qrc)
# See: https://bugreports.qt.io/browse/QTBUG-141858.
get_target_property(warn_flags warn_interface INTERFACE_COMPILE_OPTIONS)
if("-Wtrailing-whitespace=any" IN_LIST warn_flags)
  set_source_files_properties(${BITCOIN_QML_QRC} PROPERTIES COMPILE_OPTIONS -Wno-trailing-whitespace)
endif()
unset(warn_flags)

add_library(bitcoinqml STATIC
  bitcoin.cpp
  bitcoin.h
  initexecutor.cpp
  initexecutor.h
  models/nodemodel.cpp
  models/nodemodel.h
  ${BITCOIN_QML_QRC}
)

target_compile_definitions(bitcoinqml
  PUBLIC
    QT_NO_KEYWORDS
    QT_NO_CONTEXTLESS_CONNECT
    QT_USE_QSTRINGBUILDER
)

if(ENABLE_TEST_AUTOMATION)
  target_sources(bitcoinqml
    PRIVATE
      test/testbridge.cpp
      test/testbridge.h
  )
  target_compile_definitions(bitcoinqml PRIVATE ENABLE_TEST_AUTOMATION)
  target_link_libraries(bitcoinqml PUBLIC Qt6::Network)
endif()

target_link_libraries(bitcoinqml
  PRIVATE
    bitcoin_node
  PUBLIC
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

add_executable(bitcoin-qml WIN32
  main.cpp
  ../init/bitcoin-qt.cpp
)
add_windows_application_manifest(bitcoin-qml)

target_compile_definitions(bitcoin-qml
  PRIVATE
    BITCOIN_GUI_EXECUTABLE_NAME="bitcoin-qml"
)

target_link_libraries(bitcoin-qml
  PRIVATE
    core_interface
    bitcoinqml
    bitcoin_node
    $<TARGET_NAME_IF_EXISTS:bitcoin_wallet>
)

qt6_import_qml_plugins(bitcoin-qml)
import_plugins(bitcoin-qml)
install_binary_component(bitcoin-qml)

if(BUILD_GUI_QML_TESTS)
  add_subdirectory(test)
endif()
