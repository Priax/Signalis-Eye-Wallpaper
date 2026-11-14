cmake_minimum_required(VERSION 3.21)
project(signaliseyebus LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml DBus)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_qml_module(signaliseyebus
    URI SignalisEye
    VERSION 1.0
    PLUGIN_TARGET signaliseyebus
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/SignalisEye
    DEPENDENCIES QtQuick
    SOURCES eyebus.h eyebus.cpp eyecontroller.h eyecontroller.cpp
)

target_link_libraries(signaliseyebus PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::DBus)
