/**
 * @file pack_config.h
 * @brief Declarations for all static configuration data.
 * @details This file declares the structures and `extern` constants that hold
 *          the configuration data for the pack's behavior. This includes
 *          timings, colors, sound assignments, and other parameters that
 *          define the different pack modes and types. The data itself is
 *          defined in `pack_config.cpp`.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_CONFIG_H
#define PACK_CONFIG_H

#include "klystron_IO_support.h"
#include "pack_helpers.h"
#include <stdint.h>
#include <FastLED.h>

/**
 * @brief Defines heat thresholds and cooling rate for a pack type.
 */
typedef struct {
  uint16_t start_beep;     /**< Temperature at which beeping starts. */
  uint16_t start_autovent; /**< Temperature at which autovent triggers. */
  uint16_t cool_factor;    /**< Cooling multiplier relative to heating. */
} HeatSetting;

/** @brief Array of heat settings for each `PackType`. */
extern const HeatSetting pack_heat_settings[5];

/**
 * @brief Defines the LED color selection for a pack mode.
 */
typedef struct {
  CRGB powercell; /**< Powercell LED color. */
  CRGB cyclotron; /**< Cyclotron LED color. */
  CRGB future;    /**< "Future" (N-Filter) LED sequence color. */
} PackModeColor;

/** @brief Color configurations for each `PackMode`. */
extern const PackModeColor pack_mode_colors[8];

/** @brief Sound index for the short powerup sound of each pack type. */
extern const uint8_t pack_short_powerup_sounds[5];

/** Powercell pattern identifiers used in configuration tables. */
enum {
    PC_PATTERN_SHUTDOWN = 0,
    PC_PATTERN_INSTANT_OFF = 1
};

/** Cyclotron pattern identifiers used in configuration tables. */
enum {
    CY_PATTERN_FADE_OUT = 0,
    CY_PATTERN_RING_FADE_OUT = 1,
    CY_PATTERN_INSTANT_OFF = 2
};

/**
 * @brief Powerdown sequence description.
 * @details Specifies the sound and pattern timings used during shutdown for
 *          a given pack type.
 */
typedef struct {
    uint8_t sound;            /**< Sound index to play for powerdown. */
    uint8_t pc_pattern;       /**< Powercell pattern selector. */
    uint16_t pc_ms;           /**< Duration of the powercell pattern. */
    uint8_t cy_pattern;       /**< Cyclotron pattern selector. */
    uint16_t cy_ms;           /**< Duration of the cyclotron pattern. */
} PackSequence;

/** Powerdown LED and sound sequences per pack type. */
extern const PackSequence pack_powerdown_sequences[5];

/** @brief Maximum selectable song index via the song switch. */
extern const uint8_t pack_song_count;

/** @brief Minimum cycle time derived from adjustment potentiometers (ms). */
extern const uint16_t pack_adj_min_ms;

/** @brief Maximum cycle time derived from adjustment potentiometers (ms). */
extern const uint16_t pack_adj_max_ms;

/**
 * @brief Defines the set of sound indices for main activation events per pack mode.
 */
typedef struct {
  uint8_t start;     /**< Sound to play at activation start. */
  uint8_t end;       /**< Sound to play at activation end. */
  uint8_t beep_fire; /**< Sound to play for overheat warning during activation. */
  uint8_t beep_end;  /**< Sound to play for overheat stop. */
} FireSoundSet;

/** @brief Sound configurations for each pack mode and variant. */
extern const FireSoundSet pack_fire_sounds[11];

/** @brief Alignment delay in milliseconds for overheat sequences per pack mode. */
extern const uint16_t pack_sleep_align_ms[11];

/**
 * @brief Defines timing configuration for the interactive monster sounds.
 */
typedef struct {
  uint16_t response_seconds; /**< Max time allowed after a monster sound for the user to fire and get a response. */
  uint16_t min_seconds;      /**< Minimum delay between monster sound events. */
  uint16_t max_seconds;      /**< Maximum delay between monster sound events. */
} MonsterTiming;

/** @brief Timing configuration for the monster sound Easter egg. */
extern const MonsterTiming pack_monster_timing;

/** @brief The total number of monster sound/response pairs. */
extern const uint8_t pack_monster_sound_pair_count;

/** @brief Table mapping monster sound indices to their response sound indices. */
extern const uint8_t pack_monster_sound_pairs[][2];

/** @brief The number of available slime blower quote sounds. */
extern const uint8_t pack_slime_quote_count;

/** @brief The GPIO pin used for the sound module's BUSY signal. */
extern const uint8_t pack_sound_busy_pin;

/** @brief The logic level of the BUSY pin that indicates the sound module is busy. */
extern const uint8_t pack_sound_busy_level;

/** @brief The UART baud rate for communicating with the external sound module. */
extern const uint32_t pack_sound_baud_rate;

/** @brief The maximum volume level accepted by the sound module (0-30). */
extern const uint8_t pack_sound_max_volume;

/** @brief The interval for the main repeating pack timer in milliseconds. */
extern const uint32_t pack_isr_interval_ms;

#endif // PACK_CONFIG_H
