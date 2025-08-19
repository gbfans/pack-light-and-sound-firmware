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

## Features
- Power‑up sequences can be interrupted with a power‑down or fire event.
- The n‑filter supports between 1 and 16 addressable LEDs with no
  additional configuration.
- During power‑down the ADJ1 potentiometer is read to select the number of
  cyclotron LEDs (4, 24, 32 or 40).  Lights must be off, but a full power
  cycle is not required.
- Additional sound files, including mono versions for single‑speaker
  setups, are provided.  To use mono sounds copy the `mono` directory to
  the microSD card, rename your original `00` directory, then rename `mono`
  to `00`.

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


