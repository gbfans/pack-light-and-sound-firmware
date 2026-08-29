# Agent Instructions for klystron Firmware

This document provides instructions for AI software engineers working on this firmware.

## 1. Build & Flash Instructions

This project uses CMake and the Raspberry Pi Pico SDK.

### Prerequisites
1.  Install [Visual Studio Code](https://code.visualstudio.com/) and the **Raspberry Pi Pico** extension.
2.  Install the Pico SDK and set the `PICO_SDK_PATH` environment variable to its location.

### Building
1.  Open this repository in VS Code.
2.  From the command palette (`Ctrl+Shift+P`), run **Pico: Configure Project**.
3.  From the command palette, run **Pico: Build Project**.
4.  The build will produce a `klystron.uf2` firmware file inside the `build` directory.

### Flashing
1.  Hold down the **BOOTSEL** button on the Pico board.
2.  While holding the button, connect the board to your computer via USB. A mass-storage drive will appear.
3.  Copy the generated `.uf2` file to the drive. The Pico will automatically disconnect and reboot.

## 2. Test Checklist (Acceptance Criteria)

This matrix must be fully tested and passed before submitting changes. `N` refers to the active LED count set by ADJ1. The "Remainder" refers to the LEDs from `N` to 39, which must **always** be off (black, no glow).

| Setting | Condition | Expected Outcome |
| :--- | :--- | :--- |
| **Normal Mode** | ADJ1 = 4 (N=4) | Exactly 4 LEDs active (the first 4). Remainder off. |
| | ADJ1 = 24 (N=24) | Exactly 4 LEDs active, chosen via offset table from first 24. Remainder off. |
| | ADJ1 = 32 (N=32) | Exactly 4 LEDs active, chosen via offset table from first 32. Remainder off. |
| | ADJ1 = 40 (N=40) | Exactly 4 LEDs active, chosen via offset table. |
| **DIP 4x Mode** | DIP 4x = ON, N=4 | Exactly 4 single LEDs active (snap/fade). Remainder off. |
| | DIP 4x = ON, N=24 | Exactly 4 single LEDs active (snap/fade), ignoring N. Remainder off. |
| | DIP 4x = ON, N=32 | Exactly 4 single LEDs active (snap/fade), ignoring N. Remainder off. |
| | DIP 4x = ON, N=40 | Exactly 4 single LEDs active (snap/fade), ignoring N. Remainder off. |
| **Party Mode** | For each N in {4,24,32,40} | All N LEDs participate in party patterns. Remainder off. |
| **AFTERLIFE Mode**| For each N in {4,24,32,40} | All N LEDs participate in AFTERLIFE animation. Remainder off. |
| **Powercell** | Normal Operation | Powercell light scrolls from bottom to top endlessly. Does not stall. |
| **Future Light** | Firing the wand | N-Filter light activates only during firing states. |
| **TVG Lights** | Normal Operation | TVG patterns run as documented. Remainder LEDs are off. |
| **TVG Fire Timing** | Wand lights attached, ear tap (100 ms pulse) | Weapon mode changes. Nothing fires, and the wand stays in step with the pack. |
| | Wand lights attached, quick fire tap (180 ms pulse) | Short burst of firing. Mode does **not** change. |
| | Wand lights attached, fire held | Fires continuously, starting ~135 ms in. Mode does not change. |
| | Fire pulse of any length | Exactly one outcome — never both a mode change and a shot, never neither. |
| | Repeat the above with lots of LEDs lit (ADJ1 = 40, party or Afterlife animations) | Identical results. The threshold is timed off the hardware clock, so LED load must not move it. |
| | Any non-TVG pack type | Firing starts on the debounced press; taps never change mode. |


## 3. Control References

- **ADJ1 Potentiometer**: Controls the number of active LEDs in the cyclotron ring (`N`).
  - Position 1: N=4
  - Position 2: N=24
  - Position 3: N=32
  - Position 4: N=40
- **DIP Switches**:
  - **1 (PackSel0)** & **2 (PackSel1)**: Select pack variant (Classic, TVG, Afterlife).
  - **3 (Heat)**: Enables heating effects.
  - **4 (Interactive)**: Enables monster sound Easter-egg.
  - **5 (Hum)**: Enables continuous idle hum.
- **Buttons**:
  - **Fire (tap)**: In TVG mode, cycles through weapons. When a song is playing and pack is off, cycles party animations.
  - **Song Switch**: Toggles songs on/off.

## 4. Key Code Modules

- **Animation Effects (all strips)**: `SOFTWARE/animations.cpp` with the
  controller framework in `SOFTWARE/animation_controller.cpp`
- **Party Mode Logic**: `SOFTWARE/party_sequences.cpp`
- **Strip Globals/Colors**: `SOFTWARE/cyclotron_sequences.cpp`,
  `SOFTWARE/powercell_sequences.cpp`, `SOFTWARE/future_sequences.cpp`
- **Input Handling (DIPs, Pots)**: `SOFTWARE/monitors.cpp`, `SOFTWARE/klystron_IO_support.cpp`
- **State Machine**: `SOFTWARE/pack_state.cpp`
- **LED Driver Abstraction**: `SOFTWARE/addressable_LED_support.cpp`
- **Configuration Data**: `SOFTWARE/pack_config.cpp`

## 5. Concurrency Rules

The repeating-timer ISR (`pack_timer_isr`) advances the animation
controllers and pushes LED frames; the main loop runs the state machine.
When changing code, preserve these invariants:

- Only the ISR calls `show_leds()` after startup (one warm-up frame in
  `main()` precedes the timer).
- `AnimationController` mutators run in the main loop; nothing in ISR
  context may allocate or free (finished animations are retired and
  destroyed by `pack_animations_reap()` in the main loop).
- No sound-module I/O (UART writes, BUSY-pin waits) from ISR context.
- Read-modify-writes on `user_switch_flags` outside the ISR must mask
  interrupts (see the `clear_*` helpers).
