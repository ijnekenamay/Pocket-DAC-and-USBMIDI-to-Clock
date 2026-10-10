cmake_minimum_required(VERSION 3.13)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()

if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to the Raspberry Pi Pico SDK directory")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pocket_dac_usb_midi_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_executable(pocket_dac_usb_midi_clock
    src/clock_output.cpp
    src/i2s_output.cpp
    src/main.cpp
    src/usb_audio.cpp
    src/usb_descriptors.c
)

target_include_directories(pocket_dac_usb_midi_clock PRIVATE src)
target_link_libraries(pocket_dac_usb_midi_clock PRIVATE hardware_pio pico_stdlib tinyusb_device)

pico_generate_pio_header(pocket_dac_usb_midi_clock ${CMAKE_CURRENT_LIST_DIR}/src/i2s_tx.pio)

pico_enable_stdio_usb(pocket_dac_usb_midi_clock 0)
pico_enable_stdio_uart(pocket_dac_usb_midi_clock 0)
pico_add_extra_outputs(pocket_dac_usb_midi_clock)