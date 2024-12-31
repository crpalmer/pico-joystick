cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

#pico_sdk_init()

project(pico-joystick)

set(PLATFORM pico)
include(lib/platform.cmake)

add_subdirectory(lib)

add_executable(pico-joystick pico-joystick.cpp)
platform_executable(pico-joystick)
target_include_directories(pico-joystick PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pico-joystick PRIVATE lib-pi)
