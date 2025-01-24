cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

#pico_sdk_init()

function(executable name)
   add_executable(${name} ${name}.cpp pico-joystick.cpp)
   platform_executable(${name})
   target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
   target_link_libraries(${name} PRIVATE
      lib-pi
      lib-pi-net
      lib-pi-threads
      hardware_watchdog
   )
endfunction()

project(pico-joystick)

set(PLATFORM pico)
include(lib/platform.cmake)

add_subdirectory(lib)

executable(test-gamepad)
executable(joystick)
executable(spinner)
