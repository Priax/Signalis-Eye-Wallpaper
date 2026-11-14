cmake_minimum_required(VERSION 3.21)
project(signalis-eye LANGUAGES CXX)

# compile_commands.json pour clangd (l'éditeur trouve ainsi les en-têtes Qt)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(native/bus)
add_subdirectory(native/test)
