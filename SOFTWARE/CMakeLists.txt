# Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
# Licensed under the MIT License. See LICENSE file for details.

# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
set(PICO_SDK_FETCH_FROM_GIT ON)
include(pico_sdk_import.cmake)

project(klystron_v1p1 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(klystron_v1p1)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

# generate the header file into the source tree as it is included in the RP2040 datasheet
pico_generate_pio_header(klystron_v1p1 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(klystron_v1p1 PRIVATE klystron.c heat.c monster.c led_patterns.c sound.c monitors.c addressable_LED_support.c board_test.c klystron_IO_support.c sound_module.c pack.c pack_state.c powercell_sequences.c cyclotron_sequences.c future_sequences.c pack_helpers.c pack_config.c party_sequences.c)

target_link_libraries(klystron_v1p1 PRIVATE pico_stdlib hardware_gpio hardware_pio hardware_dma hardware_adc)
pico_add_extra_outputs(klystron_v1p1)

# add url via pico_set_program_url


# removing parallelversions....
# add_executable(klystron_v1p1_parallel)
# pico_generate_pio_header(klystron_v1p1_parallel ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
# target_sources(klystron_v1p1_parallel PRIVATE ws2812_parallel.c)
#target_compile_definitions(klystron_v1p1_parallel PRIVATE
#        PIN_DBG1=3)

#target_link_libraries(klystron_v1p1_parallel PRIVATE pico_stdlib hardware_pio hardware_dma)
#pico_add_extra_outputs(klystron_v1p1_parallel)


# add url via pico_set_program_url

# Additionally generate python and hex pioasm outputs for inclusion in the RP2040 datasheet
add_custom_target(klystron_v1p1_datasheet DEPENDS ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
        COMMAND pioasm -o python ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
        VERBATIM)
add_dependencies(klystron_v1p1 klystron_v1p1_datasheet)