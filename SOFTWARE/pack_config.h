/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_CONFIG_H
#define PACK_CONFIG_H

#include "klystron_IO_support.h"
#include "pack_helpers.h"
#include <stdint.h>

/**
 * @brief Heat thresholds and cooling rate for a pack type.
 */
typedef struct {
  uint16_t start_beep;     /**< Temperature at which beeping starts. */
  uint16_t start_autovent; /**< Temperature at which autovent triggers. */
  uint16_t cool_factor;    /**< Cooling multiplier relative to heating. */
} HeatSetting;

/** Array of heat settings for each pack type. */
extern const HeatSetting pack_heat_settings[5];

/**
 * @brief LED color selection for a pack mode.
 */
typedef struct {
  uint32_t powercell;    /**< Powercell LED color. */
  uint8_t cyclotron_set; /**< Cyclotron color set index. */
  uint32_t future;       /**< Future sequence color. */
} PackModeColor;

/** Color configuration for each pack mode. */
extern const PackModeColor pack_mode_colors[8];

/** Sound index for the short powerup of each pack type. */
extern const uint8_t pack_short_powerup_sounds[5];

/** Powerdown LED and sound sequences per pack type. */
extern const PackSequence pack_powerdown_sequences[5];

/**
 * @brief Maximum selectable song index.
 */
extern const uint8_t pack_song_count;

/** Minimum cycle time derived from adjustment potentiometers (ms). */
extern const uint16_t pack_adj_min_ms;

/** Maximum cycle time derived from adjustment potentiometers (ms). */
extern const uint16_t pack_adj_max_ms;

/**
 * @brief Sound indices for fire sequences per pack mode.
 */
typedef struct {
  uint8_t start;     /**< Sound to play at firing start. */
  uint8_t end;       /**< Sound to play at firing end. */
  uint8_t beep_fire; /**< Beep during firing. */
  uint8_t beep_end;  /**< Beep during firing end. */
} FireSoundSet;

/** Fire sound configuration for each pack mode and variant. */
extern const FireSoundSet pack_fire_sounds[11];

/** Alignment delay in milliseconds for overheat sequences per pack mode. */
extern const uint16_t pack_sleep_align_ms[11];

/**
 * @brief Timing configuration for interactive monster sounds.
 */
typedef struct {
  uint16_t response_seconds; /**< Max time allowed before firing to get a response. */
  uint16_t min_seconds;      /**< Minimum delay between monster sounds. */
  uint16_t max_seconds;      /**< Maximum delay between monster sounds. */
} MonsterTiming;

/** Monster sound timing configuration. */
extern const MonsterTiming pack_monster_timing;

/** Number of monster sound/response pairs. */
extern const uint8_t pack_monster_sound_pair_count;

/** Mapping of monster sound to response indices. */
extern const uint8_t pack_monster_sound_pairs[][2];

/** Number of slime blower quote sounds available. */
extern const uint8_t pack_slime_quote_count;

/** GPIO pin used for the sound module BUSY signal. */
extern const uint8_t pack_sound_busy_pin;

/** Logic level indicating the sound module is busy. */
extern const uint8_t pack_sound_busy_level;

/** UART baud rate for the external sound module. */
extern const uint32_t pack_sound_baud_rate;

/** Maximum volume level accepted by the sound module. */
extern const uint8_t pack_sound_max_volume;

/** Interval for the repeating pack timer in milliseconds. */
extern const uint32_t pack_isr_interval_ms;

#endif // PACK_CONFIG_H
