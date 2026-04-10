cmake_minimum_required(VERSION 3.22)
project(QuxlangDebuggerUI LANGUAGES CXX)

include(GNUInstallDirs)

find_package(RPNXCow REQUIRED)
find_package(RPNXMetalib REQUIRED)
find_package(RPNXSerialization REQUIRED)
find_package(RPNXDataStructures REQUIRED)
find_package(RPNXQueryGraph REQUIRED)
find_package(Quxlang REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Concurrent REQUIRED)
find_package(RPNXQueryGraphUI REQUIRED)

add_executable(quxlang-debugger-ui MACOSX_BUNDLE
        main.cpp
)

set_target_properties(quxlang-debugger-ui PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "net.rpnx.quxlang.debugger"
        MACOSX_BUNDLE_BUNDLE_NAME "Quxlang Debugger"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_STRING "Quxlang Graph Debugger"
)

target_include_directories(quxlang-debugger-ui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../quxlang/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rpnx/include
)

target_link_libraries(quxlang-debugger-ui PRIVATE
        RPNX::cow
        Quxlang::quxlang
        RPNX::querygraph-ui
)

install(TARGETS quxlang-debugger-ui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
)
