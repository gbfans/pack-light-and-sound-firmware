/**
 * @file sound.cpp
 * @brief Implements high-level sound event management.
 * @details This file provides the implementation for playing sounds that are
 *          closely tied to the pack's state and mode.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "sound.h"
#include "klystron_IO_support.h"
#include "monitors.h"
#include "pack_config.h"
#include "pack_state.h"
#include "sound_module.h"
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the sound subsystem.
 * @details This function should be called once at startup to initialize the
 *          sound module and unmute the amplifier. It includes delays to ensure
 *          the sound hardware is ready before use.
 */
void sound_startup(void) {
  sound_init();
  sleep_ms(1000);
  sound_volume(pack_sound_max_volume);
  sleep_ms(50);
  unmute_audio();
  sleep_ms(50);
}

/**
 * @brief Manages the sound effects for the main activation sequence.
 * @details Plays a sound associated with the current pack's main activation
 *          (e.g., firing, overheat). The specific sound played is determined
 *          by the current `PackMode` and the `fire_type` index, using the
 *          `pack_fire_sounds` configuration table.
 * @param fire_type An index indicating the type of event:
 *                  - 0: Start/continue activation sound.
 *                  - 1: End activation sound.
 *                  - 2: Overheat warning sound during activation.
 *                  - 3: Stop sound due to overheat.
 */
void fire_department(uint8_t fire_type) {
  PackType type = config_pack_type();
  uint8_t _pack_index = 0;
  switch (type) {
  case PACK_TYPE_SNAP_RED: // 4x cyclotron Red only Snap
    _pack_index = 8;
    break;
  case PACK_TYPE_FADE_RED: // 4x cyclotron Red only fade
    _pack_index = 9;
    break;
  case PACK_TYPE_TVG_FADE:                     // 4x cyclotron TVG fade
  case PACK_TYPE_AFTER_TVG:                    // Afterlife TVG
    _pack_index = pack_state_get_mode(); // modes 0 through 7
    break;
  default: // Afterlife
    _pack_index = 10;
  }
  const FireSoundSet *set = &pack_fire_sounds[_pack_index];
  uint8_t sound = 0;
  switch (fire_type) {
  case 0:
    sound = set->start;
    break;
  case 1:
    sound = set->end;
    break;
  case 2:
    sound = set->beep_fire;
    break;
  case 3:
    sound = set->beep_end;
    break;
  default:
    return;
  }
  if (sound > 0) {
    sound_start_safely(sound);
    if (fire_type == 0) {
      sleep_ms(750);
      if (((type == PACK_TYPE_TVG_FADE) || (type == PACK_TYPE_AFTER_TVG)) &&
          (_pack_index & 0x01)) {
        sound_wait_til_end(false, false);
      }
    }
  }
}

/**
 * @brief Delays execution to align wand lights with sound during overheat.
 * @details This is a blocking delay whose duration is determined by the
 *          current pack mode, looked up from the `pack_sleep_align_ms`
 *          configuration table. It's used to synchronize visual effects with
 *          specific sound cues.
 */
void sleep_align_wandlights(void) {
  PackType type = config_pack_type();
  uint8_t _pack_index = 0;
  switch (type) {
  case PACK_TYPE_SNAP_RED:
    _pack_index = 8;
    break;
  case PACK_TYPE_FADE_RED:
    _pack_index = 9;
    break;
  case PACK_TYPE_TVG_FADE:
  case PACK_TYPE_AFTER_TVG:
    _pack_index = pack_state_get_mode();
    break;
  default:
    _pack_index = 10;
  }
  sleep_ms(pack_sleep_align_ms[_pack_index]);
}

#ifdef __cplusplus
}
#endif
