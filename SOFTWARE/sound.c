/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "sound.h"
#include "klystron_IO_support.h"
#include "monitors.h"
#include "pack_config.h"
#include "pack_state.h"
#include "sound_module.h"
#include "pico/stdlib.h"

/**
 * @brief Initialize sound hardware and set default volume.
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
 * @brief Play a fire-related sound for the current pack mode.
 *
 * @param sound_index Index into the fire sound set.
 */
void fire_department(uint8_t sound_index) {
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
  switch (sound_index) {
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
    if (sound_index == 0) {
      sleep_ms(750);
      if (((type == PACK_TYPE_TVG_FADE) || (type == PACK_TYPE_AFTER_TVG)) &&
          (_pack_index & 0x01)) {
        sound_wait_til_end(false, false);
      }
    }
  }
}

/**
 * @brief Delay to align wand lights during overheat sequences.
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
