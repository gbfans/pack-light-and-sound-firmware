<!--
Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
Licensed under the MIT License. See LICENSE file for details.
-->

# GBFans.com Pack Light and Sound Firmware

[![Build](https://github.com/gbfans/pack-light-and-sound-firmware/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/gbfans/pack-light-and-sound-firmware/actions/workflows/cmake-single-platform.yml)

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
- **N-Filter Vent Light**: The "Future" light (the 16 LEDs on the N-Filter) is the vent light: it strobes (or, on Afterlife packs, rotates) for the duration of a vent sequence. At the same time the vent relay output (GPIO 28, the RELAY connector) is held on so smoke machines and other vent effects trigger cleanly.
- **Party Mode**: Activate party mode by starting a song with the song switch, then tapping the fire button while the pack is off. This will cycle through several fun animations that use all `N` cyclotron LEDs.
- Power‑up sequences can be interrupted with a power‑down or fire event.
- Additional sound files, including mono versions for single‑speaker
  setups, are provided.

## Firmware architecture
The firmware source lives in the [`SOFTWARE`](SOFTWARE) directory and targets
the Raspberry Pi Pico. The `klystron.cpp` entry point configures GPIO, LED
drivers and the serial sound board, then starts a repeating timer whose ISR
polls the switches, advances the LED animations and pushes frames to the
strips. The main loop runs the pack state machine (`pack_state_process()`).

### State machine
`pack_state.cpp` defines the high‑level states that coordinate lights, sounds
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
| **PS_FEEDBACK** | Brief confirmation on the cyclotron ring when ADJ1 changes while the pack is off: solid red = 4 LEDs, solid green = 24, solid blue = 32, scrolling rainbow = 40. |

### Modules
- `pack_config.cpp` holds the static configuration tables (colors, sounds, heat).
- `monitors.cpp` watches user inputs and selects the cyclotron LED count.
- `animations.cpp` implements the LED effects, driven by the
  `AnimationController` framework; `addressable_LED_support.cpp` wraps the
  FastLED driver and masks unused cyclotron LEDs on every frame.
- `sound_module.cpp` talks to the external serial sound board while
  `sound.cpp` coordinates cues.
- Optional effects such as heating and the monster Easter‑egg live in
  `heat.cpp` and `monster.cpp`.

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
   without duplicating code. The Cylon effect draws a single bright "eye"
   that wraps around the strip, or bounces end to end when `bounce` is set;
   `ScrollAnimation` fills the strip one LED at a time and then clears.

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
simulator in the [`SOFTWARE/sim`](SOFTWARE/sim) folder, which compiles the
firmware's own animation sources so the previews show exactly what the pack
renders. The GitHub Actions workflow
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
- **ADJ0** – sets the powercell animation speed. The cyclotron ring runs at
  a fixed rate (Afterlife packs ramp it during startup, firing and
  cooldown).
- **ADJ1** – selects the number of cyclotron LEDs. Turning it while the pack
  is off shows a confirmation on the ring: **solid red = 4**, **solid
  green = 24**, **solid blue = 32**, **scrolling rainbow = 40** — so a ring
  with only 4 physical LEDs still shows unambiguously which setting is
  selected (any color other than solid red means keep turning). Firmware
  prior to v1.2.0 showed the scrolling rainbow at every setting, which was
  easy to misread as "confirmed" on smaller rings.

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

#### Fire button timing in TVG modes
In a TVG mode a press on the fire input is ambiguous until it either ends or
outlasts the **135 ms tap window**:

- The pulse **ends inside** the window → mode change; nothing fires.
- The pulse is **still active at the end** of the window → firing starts right
  then, and no mode change happens.

The two outcomes are exactly complementary: a press can never do both, and
never do nothing. Because of that, firing in a TVG mode is always held off for
the length of the window — there is no way to know which request a press is
until it resolves.

The figure comes from the Wand Lights board, which conditions the line rather
than passing the button through: the ear button emits a fixed 100 ms pulse and
the fire button is stretched to at least 180 ms. Sweeping the threshold across
poll intervals of 4 to 6 ms and every phase alignment of the pulse against the
poll grid, a 100 ms pulse stays a mode change from 97 ms upwards and a 180 ms
pulse still fires up to 173 ms; 135 ms is the midpoint of that band, so the
margin is an equal 38 ms on each side. Rebuild with
`-DFIRE_TAP_WINDOW_MS=<value>` to re-sweep it against real hardware. Wired
directly to a switch the same threshold applies, and the tap has to be made by
hand.

In every non-TVG pack type there are no weapon modes to cycle, so firing starts
as soon as the fire contact is debounced (about 12 ms) and the window is not
used at all.

Widths are measured against the hardware microsecond timer, not by counting
passes through the pack timer ISR. The ISR is armed with a positive delay,
which the Pico SDK defines as the gap between one callback ending and the next
starting, and the same callback drives the LED output — so the interval between
two polls is 4 ms plus however long the previous pass took, and it moves with
LED load. Timestamping keeps the threshold at a true 135 ms; the poll rate only
sets the resolution, so the boundary lands within one poll of 135 ms no matter
what the animations are doing.

## Building with Visual Studio Code

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the
   **Raspberry Pi Pico** extension.
2. Install the Pico SDK and set the `PICO_SDK_PATH` environment variable to
   its location.
3. Open this repository in VS Code. From the command palette (`Ctrl+Shift+P`)
   run **Pico: Configure Project** and then **Pico: Build Project**. The build
   produces a `.uf2` firmware file inside the `build` directory.

## Flashing firmware

1. Make sure all battery power connections are disconnected from the board.
2. Hold down the **BOOTSEL** button on the Pico.
3. While holding the button, connect the board to your computer with USB. A
   mass‑storage drive appears.
4. Copy the generated `.uf2` file to the drive. The Pico automatically
   disconnects after the copy completes.
5. Wait a moment for the copy to finish, then unplug the board from your
   computer. Flashing is complete and battery power may now be reconnected.


