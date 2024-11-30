cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico-joystick)

# include(lib/platform.cmake)

add_library(pico-joystick STATIC
  pico-joystick.cpp
  ${LIB_OBJS}
)

target_include_directories(pico-joystick PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico-joystick PUBLIC
   pico_btstack
   pico_btstack_ble
   pico_btstack_classic
   pico_btstack_cyw43
)
#lib-pi)
