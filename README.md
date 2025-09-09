<!--
Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
Licensed under the MIT License. See LICENSE file for details.
-->

# GBFans.com Pack Light and Sound Firmware

[![Build](https://github.com/ajquick/klystron/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/ajquick/klystron/actions/workflows/cmake-single-platform.yml)

The GBFans.com Pack Light and Sound Firmware is a firmware package for
controlling Ghostbusters proton pack lighting and sound effects. It runs
on the GBFans.com pack controller and manages the cyclotron ring, power
cell, n-filter LEDs and synchronized sound playback for multiple pack
styles.

## Pack modes
- **Classic/Red** – standard movie pack behavior.
- **Afterlife TVG** – when *Afterlife* is selected and heating effects are
  enabled, the pack enters Afterlife TVG mode. The LED ring changes color
  to match each of the eight available pack modes. Without heating effects
  the ring operates in red only, as in the original Afterlife behavior.
- **TVG** – traditional video game mode with its own color schemes.

### Mode Behavior
The number of active LEDs in the cyclotron ring (`N`) is determined by the `ADJ1` potentiometer. The behavior of the cyclotron changes based on the selected pack mode. In all modes, any LEDs beyond `N` are kept off.

| Mode | DIP Switch Setting | Cyclotron LED Driver |
| :--- | :--- | :--- |
| **Normal (4x Snap/Fade)** | `00` or `01` | **4 single LEDs** are lit, based on an offset table. The `N` setting is ignored. |
| **Normal (TVG / Afterlife)** | `10` or `11` | **4 LEDs** are lit, based on an offset table that scales with `N`. |
| **Party Mode** | See below | **All N LEDs** are used for the selected party animation. |
| **AFTERLIFE Animation** | `11` (during cooldown) | **All N LEDs** are used for the AFTERLIFE-specific animation. |

## Features
- **Adjustable Cyclotron Ring (N)**: The `ADJ1` potentiometer actively controls the number of logical LEDs in the cyclotron ring. Allowed values are 4, 24, 32, and 40. All animations and modes respect this setting, and any LEDs beyond the selected count (`N`) are always forced off.
- **Powercell Endless Scroll**: The powercell light bar now scrolls from bottom to top in a continuous, endless loop during normal operation.
- **N-Filter Firing Light**: The "Future" light (the 16 LEDs on the N-Filter) is now correctly tied to the pack's firing state and is no longer activated by the vent sequence.
- **Party Mode**: Activate party mode by starting a song with the song switch, then tapping the fire button while the pack is off. This will cycle through several fun animations that use all `N` cyclotron LEDs.
- Power‑up sequences can be interrupted with a power‑down or fire event.
- Additional sound files, including mono versions for single‑speaker
  setups, are provided.

## Firmware architecture
The firmware source lives in the [`SOFTWARE`](SOFTWARE) directory and targets
the Raspberry Pi Pico. The `klystron.c` entry point configures GPIO, LED
drivers and the serial sound board before a repeating timer calls into the
pack state machine.

### State machine
`pack_state.c` defines the high‑level states that coordinate lights, sounds
and optional effects:

| State | Purpose |
|-------|---------|
| **PS_OFF** | Pack is powered down; LEDs are blank and inputs are monitored for a power‑up request. |
| **PS_PACK_STANDBY** | Short standby after a pack‑only power‑up. |
| **PS_WAND_STANDBY** | Wand is active but the pack has not fully powered. |
| **PS_IDLE** | Normal running state with idle light animations and hum. |
| **PS_FIRE** | Main firing state for the proton stream and other modes. |
| **PS_FIRE_COOLDOWN** | Afterlife packs briefly slow the cyclotron after firing. |
| **PS_SLIME_FIRE** | Firing state for slime‑blower and tether modes. |
| **PS_OVERHEAT** | Heating effects have triggered an overheat. |
| **PS_OVERHEAT_BEEP** | Cooling period with warning beeps. |
| **PS_AUTOVENT** | Automatic vent sequence once the pack has overheated. |

### Modules
- `pack_config.c` reads DIP switches and potentiometers.
- `monitors.c` debounces user inputs and selects the cyclotron LED count.
- `addressable_LED_support.c` with the various `*_sequences.c` files drive the LED patterns.
- `sound_module.c` talks to the external serial sound board while
  `sound.c` coordinates cues.
- Optional effects such as heating and the monster Easter‑egg live in
  `heat.c` and `monster.c`.

## Creating new light patterns

The firmware uses a small animation framework to make it easier to build and
compose LED effects. Each animation derives from the base `Animation` class in
[`SOFTWARE/animation.h`](SOFTWARE/animation.h) and is driven by an
`AnimationController` instance.

### Defining an animation

1. Create a new subclass of `Animation` in
   [`SOFTWARE/animations.h`](SOFTWARE/animations.h) and implement the
   `start`, `update`, and `isDone` methods in
   [`SOFTWARE/animations.cpp`](SOFTWARE/animations.cpp).
2. Use the provided `AnimationConfig` struct to describe the LED buffer, count,
   base color and starting speed for the pattern. Additional flags such as
   `bounce` can adjust behavior for generic animations like the Cylon scanner
   without duplicating code. The Cylon effect always draws the trailing "eye";
   for a simple moving dot use `ScrollAnimation`.

### Playing an animation

1. Obtain a reference to an `AnimationController` (for example
   `g_powercell_controller`).
2. Create a `PlayAnimationAction` with your animation instance and desired
   `AnimationConfig` and enqueue it on the controller.
3. The controller's `update` method should be called regularly; the main pack
   loop already updates the global controllers for you.

### Modifying a running animation

Actions such as `ChangeColorAction` and `ChangeSpeedAction` can be enqueued to
smoothly adjust the current animation's color or speed over a specified
duration. These modifiers make it easy to ramp colors, fade to black or
increase the scroll rate without replacing the underlying animation. Each
action accepts an optional [`ramp_mode`](SOFTWARE/libs/RAMP/Ramp.h) parameter
allowing transitions to use easing curves such as `QUADRATIC_INOUT` or
`CUBIC_OUT` instead of the default `LINEAR` ramp.

## Animation rendering

Preview videos for the built-in animations can be generated using the
simulation helpers in the [`SOFTWARE/sim`](SOFTWARE/sim) folder. The GitHub Actions workflow
`render-animations.yml` iterates over the entries in
[`SOFTWARE/sim/animation_configs.json`](SOFTWARE/sim/animation_configs.json) and records each
listed animation with its specified LED count, color, and layout. The layout
field accepts `ring` for circular arrangements or `strip` for linear light
strips. To add an animation to the rendered set, append a new object to this
JSON file.

Rendered animation GIFs are stored in the [SOFTWARE/animations](SOFTWARE/animations) directory.
For a complete gallery grouped by light type, LED count, and mode, see
[ANIMATIONS.md](ANIMATIONS.md).

## Configuration

### CONFIG dip switches
| Switch | Function |
|--------|----------|
| 1 – **PackSel0** | Together with switch 2 selects the pack variant.<br>00 = 4× red snap, 01 = 4× red fade, 10 = TVG, 11 = Afterlife (or Afterlife TVG when switch 3 is on). |
| 2 – **PackSel1** | Second bit of the pack selection. |
| 3 – **Heat** | Enables heating effects and allows Afterlife TVG when used with switches 1 and 2 both on. |
| 4 – **Interactive** | Enables the monster sound Easter‑egg. |
| 5 – **Hum** | Plays a continuous idle hum track. |

### Potentiometers
- **ADJ0** – sets the default speed of the pack light sequences (does not
  affect cyclotron ring speed in Afterlife configurations).
- **ADJ1** – selects the number of cyclotron LEDs.

## Test mode
To enter test mode, set all CONFIG dip switches to **ON** and hold both the
FIRE and SONG inputs active while applying power. Press **FIRE** to advance
through each step. To exit the switch‑testing step, turn all CONFIG dip
switches **OFF** and press **FIRE** again.

All pack modes operate with 4, 24, 32 or 40 cyclotron LEDs by selecting the
appropriate value with ADJ1.

### Recommended Pack/Wand Lights settings
| Pack Description | Pack Sel0 | Pack Sel1 | Heating | Wand Lights Description | 1 | 2 | 3 |
|------------------|-----------|-----------|---------|-------------------------|---|---|---|
| 4x Red Snap          | OFF       | OFF       | OFF     | Movie Wand              | OFF | OFF | X |
| 4x Red Fade          | ON        | OFF       | OFF     | Movie Wand              | OFF | OFF | X |
| Afterlife Red        | ON        | ON        | OFF     | Movie Wand              | OFF | OFF | X |
| 4x Red Snap          | OFF       | OFF       | ON      | Movie Wand w/Heating    | OFF | ON  | ON |
| 4x Red Fade          | ON        | OFF       | ON      | Movie Wand w/Heating    | OFF | ON  | ON |
| Afterlife Red        | ON        | ON        | ON      | Movie Wand w/Heating    | OFF | ON  | ON |
| 4x TVG               | OFF       | ON        | ON      | TVG Wands               | ON  | ON  | X |

`X` indicates a setting that is not used.

### TVG Lights
When the pack is set to a TVG mode (via `PackSel0`/`PackSel1` DIP switches), the fire button can be tapped (when not firing) to cycle through different weapon modes. Each mode has a unique color for the cyclotron and powercell, as defined in the firmware. These modes include the Proton Stream, Slime Blower, Stasis Stream, and more. The TVG lights respect the `N` setting from the ADJ1 potentiometer, meaning only the selected number of LEDs will be active.

## Building with Visual Studio Code

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the
   **Raspberry Pi Pico** extension.
2. Install the Pico SDK and set the `PICO_SDK_PATH` environment variable to
   its location.
3. Open this repository in VS Code. From the command palette (`Ctrl+Shift+P`)
   run **Pico: Configure Project** and then **Pico: Build Project**. The build
   produces a `.uf2` firmware file inside the `build` directory.

## Flashing firmware

1. Hold down the **BOOTSEL** button on the Pico.
2. While holding the button, connect the board to your computer with USB. A
   mass‑storage drive appears.
3. Copy the generated `.uf2` file to the drive. The Pico automatically
   disconnects after the copy completes.
4. Wait a moment for the copy to finish, then unplug the board from your
   computer.


