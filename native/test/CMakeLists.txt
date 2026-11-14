cmake_minimum_required(VERSION 3.21)
project(signaliseyegrab LANGUAGES CXX)
find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick)
qt_standard_project_setup()
qt_add_executable(grab grab.cpp)
target_compile_definitions(grab PRIVATE SCENE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../wallpaper/contents/ui/Scene.qml")
target_link_libraries(grab PRIVATE Qt6::Gui Qt6::Quick)

qt_add_executable(bench bench.cpp)
target_compile_definitions(bench PRIVATE SCENE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../wallpaper/contents/ui/Scene.qml")
target_link_libraries(bench PRIVATE Qt6::Gui Qt6::Quick)

foreach(tool grab bench)
    target_compile_definitions(${tool} PRIVATE
        PLUGIN_IMPORT_PATH="${CMAKE_BINARY_DIR}/native/bus")
    add_dependencies(${tool} signaliseyebus)
endforeach()
