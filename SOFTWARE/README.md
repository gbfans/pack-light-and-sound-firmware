<!--
Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
Licensed under the MIT License. See LICENSE file for details.
-->

# GBFans.com Pack Light and Sound Firmware

This directory contains the source code for the GBFans.com pack light and sound controller firmware. The firmware targets the [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) and drives the lighting and sound effects of the pack.

## Architecture
- **`klystron.cpp`** – application entry point. Initializes hardware peripherals, sets up LED drivers and the serial sound module, then starts a repeating timer. The timer ISR (`pack_timer_isr`) debounces inputs, advances LED animations and updates heat and monster timers. The main loop runs the pack state machine via `pack_state_process()`.
- **State machine** – `pack_state.cpp/h` defines high‑level states such as standby, firing, cooldown and autovent. `pack.cpp` coordinates the startup and shutdown sequences.
- **Configuration and monitoring** – `pack_config.cpp` holds the configuration tables, while `monitors.cpp` watches user inputs and determines the selected cyclotron ring size. `board_test.cpp` enables a diagnostic routine when all configuration switches are on.

### LED control
- **`addressable_LED_support.cpp/h`** wrap the FastLED RP2040 driver (PIO + DMA) for the WS2812‑style LED strips and mask unused cyclotron LEDs on every frame.
- The animation effects live in `animations.cpp`, driven by `animation_controller.cpp`; `party_sequences.cpp` orchestrates party mode across all three strips, and `led_patterns.cpp` maps pack modes to colors.

### Sound
- **`sound_module.cpp`** implements a UART protocol to an external serial sound board. Higher‑level cues are defined in `sound.cpp`.

### Effects
- **`heat.cpp`** and **`monster.cpp`** implement optional heating and monster Easter‑egg effects.

## Building
The project uses CMake and the Raspberry Pi Pico SDK.

```bash
cd SOFTWARE
mkdir build && cd build
cmake ..    # fetches the Pico SDK if required
make
```

`cmake` looks for the ARM GCC toolchain (`arm-none-eabi-gcc`). Set `PICO_TOOLCHAIN_PATH` if the compiler is installed in a non‑standard location. The resulting `.uf2` firmware file appears in the `build` directory and can be copied to the Pico's USB mass‑storage device to flash the controller.

## Sound files
Mono and alternate sound banks can be copied to the microSD card as described in the repository's top‑level README.

