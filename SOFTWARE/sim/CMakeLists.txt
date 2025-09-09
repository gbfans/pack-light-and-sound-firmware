cmake_minimum_required(VERSION 3.16)
project(sim_led)

set(CMAKE_CXX_STANDARD 17)
add_executable(sim_led
  simulator_main.cpp
  example_sketch.cpp
)
target_include_directories(sim_led PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
