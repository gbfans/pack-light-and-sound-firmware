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
set(PICO_PLATFORM rp2040 CACHE STRING "Platform")
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
set(PICO_SDK_FETCH_FROM_GIT ON)
set(PICO_SDK_FETCH_SDK_VERSION 2.1.1)
include(pico_sdk_import.cmake)

project(klystron VERSION 1.2.0 LANGUAGES C CXX)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PICO_SDK_PATH}/src/common/pico_base_headers/include ${PICO_SDK_PATH}/src/rp2040/pico_platform/include ${PICO_SDK_PATH}/src/rp2_common/pico_platform_compiler/include ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include ${PICO_SDK_PATH}/src/rp2_common/pico_platform_sections/include ${PICO_SDK_PATH}/src/rp2_common/pico_platform_panic/include ${PICO_SDK_PATH}/src/rp2_common/pico_platform_common/include ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include ${PICO_SDK_PATH}/src/rp2_common/hardware_sync/include ${CMAKE_CURRENT_BINARY_DIR}/generated/pico_base)
include_directories(${PICO_SDK_PATH}/src/common/pico_stdlib_headers/include)
include_directories(${PICO_SDK_PATH}/src/common/pico_stdio/include)

# SDK 2.1.x split spin_lock into its own component; make headers visible to all targets
if (EXISTS "${PICO_SDK_PATH}/src/rp2_common/hardware_sync_spin_lock/include")
  include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_sync_spin_lock/include)
endif()

# Add FastLED library
add_subdirectory(libs/FastLED)
# Make FastLED compile with the Pico SDK usage requirements
if (TARGET fastled)

  target_link_libraries(fastled PUBLIC
    pico_stdlib
    hardware_gpio
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_timer
    hardware_clocks
    hardware_sync
  )
  
  # Ensure FastLED OBJ libs see Pico component headers AND RP2040 defines
  set(_fastled_objs FastLED_core FastLED_fx FastLED_fl FastLED_platforms FastLED_sensors FastLED_third_party)
  foreach(_t IN LISTS _fastled_objs)
    if (TARGET ${_t})
      # include dirs you already had
      foreach(_c hardware_gpio hardware_pio hardware_dma hardware_irq hardware_timer hardware_clocks hardware_sync hardware_structs pico_stdio)
        if (TARGET ${_c})
          target_include_directories(${_t} PRIVATE
            $<TARGET_PROPERTY:${_c},INTERFACE_INCLUDE_DIRECTORIES>)
        endif()
      endforeach()
      if (TARGET hardware_sync_spin_lock)
        target_include_directories(${_t} PRIVATE
          $<TARGET_PROPERTY:hardware_sync_spin_lock,INTERFACE_INCLUDE_DIRECTORIES>)
      endif()
  
      # 🔧 add these lines:
      target_compile_definitions(${_t} PRIVATE FASTLED_RP2040=1 PICO_RP2040=1)
      
      # Select the RP2040 platform in FastLED's platforms.h and provide Arduino API shims
      target_compile_definitions(${_t} PRIVATE ARDUINO=100 ARDUINO_ARCH_RP2040=1)
      target_compile_options(${_t} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/Arduino.h>
      )
      
    endif()
  endforeach()
  
  # (Harmless) keep this too for consumers of the final static lib
  if (TARGET fastled)
    target_compile_definitions(fastled PUBLIC FASTLED_RP2040=1 PICO_RP2040=1)
  endif()
endif()

if (EXISTS "${PICO_SDK_PATH}/src/rp2_common/hardware_gpio/include")
  include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_gpio/include)
endif()
include_directories(${PICO_SDK_PATH}/src/rp2040/hardware_structs/include)

# Add RAMP library
add_library(RAMP libs/RAMP/Ramp.cpp libs/RAMP/RampCRGB.cpp)
target_include_directories(RAMP PUBLIC libs/RAMP . libs/FastLED)
target_link_libraries(RAMP PUBLIC pico_stdlib m fastled)
set_source_files_properties(libs/RAMP/Ramp.cpp libs/RAMP/RampCRGB.cpp PROPERTIES COMPILE_OPTIONS "-include${CMAKE_CURRENT_SOURCE_DIR}/Arduino.h")
target_compile_definitions(RAMP PUBLIC FASTLED_RP2040=1 PICO_RP2040=1 ARDUINO=100 ARDUINO_ARCH_RP2040=1)

# Sources shared by every firmware image
set(KLYSTRON_SOURCES
  klystron.cpp heat.cpp monster.cpp led_patterns.cpp sound.cpp monitors.cpp
  addressable_LED_support.cpp board_test.cpp klystron_IO_support.cpp
  sound_module.cpp pack.cpp pack_state.cpp powercell_sequences.cpp
  cyclotron_sequences.cpp future_sequences.cpp pack_helpers.cpp pack_config.cpp
  party_sequences.cpp animations.cpp animation_controller.cpp action.cpp)

# Define one firmware image. Any extra arguments are added as private compile
# definitions, which is how the build spins the static-ring variants below out
# of the same sources.
function(add_klystron_firmware target)
  add_executable(${target})

  target_sources(${target} PRIVATE ${KLYSTRON_SOURCES})

  # Make the app see RP2040 + Arduino shim too
  target_compile_definitions(${target} PRIVATE
    FASTLED_RP2040=1
    PICO_RP2040=1
    ARDUINO=100
    ARDUINO_ARCH_RP2040=1
    FW_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FW_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FW_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ${ARGN}
  )

  # Only C++ files should auto-include Arduino.h
  target_compile_options(${target} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/Arduino.h>
  )

  target_include_directories(${target} PRIVATE "." "libs" "libs/FastLED" "libs/RAMP")

  target_link_libraries(${target} PRIVATE pico_stdlib hardware_gpio hardware_adc hardware_dma hardware_pio hardware_irq hardware_timer hardware_clocks hardware_sync fastled RAMP m)
  pico_add_extra_outputs(${target})
endfunction()

# Stock firmware: both adjustment potentiometers live.
add_klystron_firmware(klystron)

# Static-ring firmware for packs whose adjustment pots have failed or been
# removed. These builds never read the ADC: the cyclotron ring size is fixed
# at the number in the file name and the ADJ0 speed adjustment is gone too
# (see build_options.h). Every image is built alongside the stock firmware, so
# `cmake --build build` and the CI Build workflow produce all five .uf2 files.
set(KLYSTRON_STATIC_LED_COUNTS 4 24 32 40)
foreach(led_count IN LISTS KLYSTRON_STATIC_LED_COUNTS)
  # Zero-pad single digits so the images sort and read consistently:
  # klystron_static_04_leds.uf2 ... klystron_static_40_leds.uf2
  if(led_count LESS 10)
    set(led_label "0${led_count}")
  else()
    set(led_label "${led_count}")
  endif()
  add_klystron_firmware(klystron_static_${led_label}_leds
    STATIC_CYCLOTRON_LED_COUNT=${led_count})
endforeach()
