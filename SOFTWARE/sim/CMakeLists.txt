cmake_minimum_required(VERSION 3.16)
project(sim_led)

set(CMAKE_CXX_STANDARD 17)

# The simulator renders the firmware's own animation code on the host. The
# sim directory is listed first so <FastLED.h> resolves to the host shim
# (FastLED.h here) instead of the RP2040-only library, and Arduino.h (the
# virtual clock) is force-included the way the firmware build force-includes
# its Pico Arduino shim.
add_executable(sim_led
  sim_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../animations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../animation_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../action.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../party_sequences.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../cyclotron_sequences.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../powercell_sequences.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../future_sequences.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../libs/RAMP/Ramp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../libs/RAMP/RampCRGB.cpp
)
target_include_directories(sim_led BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sim_led PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../libs/RAMP
)
target_compile_options(sim_led PRIVATE -include${CMAKE_CURRENT_SOURCE_DIR}/Arduino.h)
