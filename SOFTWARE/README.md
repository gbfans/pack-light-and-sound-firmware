<!--
Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
Licensed under the MIT License. See LICENSE file for details.
-->

# GBFans.com Pack Light and Sound Firmware

This directory contains the source code for the GBFans.com pack light and sound controller firmware. The firmware targets the [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) and drives the lighting and sound effects of the pack.

## Architecture
- **`klystron.c`** – application entry point. Initializes hardware peripherals, sets up LED drivers and the serial sound module, then starts a repeating timer. The timer ISR (`pack_timer_isr`) debounces inputs, advances LED animations and updates heat and monster timers. The main loop runs the pack state machine via `pack_state_process()`.
- **State machine** – `pack_state.c/h` defines high‑level states such as standby, firing, cooldown and autovent. `pack.c` and helpers in `pack_helpers.c` coordinate transitions and mode‑specific behaviour.
- **Configuration and monitoring** – `pack_config.c` reads DIP switches and potentiometers, while `monitors.c` watches user inputs and determines the selected cyclotron ring size. `board_test.c` enables a diagnostic routine when all configuration switches are on.

### LED control
- **`addressable_LED_support.c/h`** set up PIO state machines and DMA channels to drive WS2812‑style LED strips.
- Animation sequences live in `powercell_sequences.c`, `cyclotron_sequences.c`, `future_sequences.c` and `party_sequences.c`. `led_patterns.c` contains low‑level pattern helpers.

### Sound
- **`sound_module.c`** implements a UART protocol to an external serial sound board. Higher‑level cues are defined in `sound.c`, and `sound_module` ensures playback is synchronised with pack events.

### Effects
- **`heat.c`** and **`monster.c`** implement optional heating and monster Easter‑egg effects.

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

