/**
 * @file pack_config.cpp
 * @brief Definitions for all static configuration data.
 * @details This file defines the constant data structures that hold the
 *          configuration data for the pack's behavior. This includes timings,
 *          colors, sound assignments, and other parameters.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack_config.h"
#include "addressable_LED_support.h"
#include "pack_state.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"

/** @brief Heat settings for each pack type: {start_beep, start_autovent, cool_factor}. */
const HeatSetting pack_heat_settings[5] = {
    {6 * 250, 10 * 250, 1}, /**< [0] PACK_TYPE_SNAP_RED */
    {7 * 250, 11 * 250, 1}, /**< [1] PACK_TYPE_FADE_RED */
    {8 * 250, 13 * 250, 1}, /**< [2] PACK_TYPE_TVG_FADE */
    {7 * 250, 11 * 250, 1}, /**< [3] PACK_TYPE_AFTERLIFE */
    {8 * 250, 13 * 250, 1}, /**< [4] PACK_TYPE_AFTER_TVG */
};

/** @brief Sound index for the short power-up sound for each pack type. */
const uint8_t pack_short_powerup_sounds[5] = {93, 94, 94, 124, 124};

/**
 * @brief Powerdown light and sound sequences for each pack type.
 * @details Classic snap packs power down instantly while fade and TVG variants
 *          use a fade-out. Afterlife variants use a ring-specific fade routine.
 */
const PackSequence pack_powerdown_sequences[5] = {
    [PACK_TYPE_SNAP_RED]  = {11, PC_PATTERN_INSTANT_OFF, 0, CY_PATTERN_INSTANT_OFF, 0},
    [PACK_TYPE_FADE_RED]  = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_FADE_OUT, 2900},
    [PACK_TYPE_TVG_FADE]  = {59, PC_PATTERN_SHUTDOWN, 3100, CY_PATTERN_FADE_OUT, 3100},
    [PACK_TYPE_AFTERLIFE] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_FADE_OUT, 2900},
    [PACK_TYPE_AFTER_TVG] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_FADE_OUT, 2900},
};

/** @brief Number of selectable songs available via the song switch. */
const uint8_t pack_song_count = 3;

/** @brief Minimum ADJ-derived cycle time in milliseconds. */
const uint16_t pack_adj_min_ms = 400;

/** @brief Maximum ADJ-derived cycle time in milliseconds. */
const uint16_t pack_adj_max_ms = 1300;

/** @brief LED color selections for each pack mode. */
const PackModeColor pack_mode_colors[8] = {
    [PACK_MODE_PROTON_STREAM] = {CRGB::Blue,   CRGB::Red,    CRGB::White},
    [PACK_MODE_BOSON_DART]    = {CRGB::Blue,   CRGB::Red,    CRGB::White},
    [PACK_MODE_SLIME_BLOWER]  = {CRGB::Green,  CRGB::Green,  CRGB::White},
    [PACK_MODE_SLIME_TETHER]  = {CRGB::Green,  CRGB::Green,  CRGB::White},
    [PACK_MODE_STASIS_STREAM] = {CRGB::Blue,   CRGB::Blue,   CRGB::White},
    [PACK_MODE_SHOCK_BLAST]   = {CRGB::Blue,   CRGB::Blue,   CRGB::White},
    [PACK_MODE_OVERLOAD_PULSE]= {CRGB::Orange, CRGB::Orange, CRGB::White},
    [PACK_MODE_MESON_COLLIDER]= {CRGB::Orange, CRGB::Orange, CRGB::White},
};

/** @brief Sound assignments for activation events for each mode and variant. */
const FireSoundSet pack_fire_sounds[11] = {
    {19, 16, 20, 18},     /**< [0] Proton Stream */
    {21, 0, 22, 0},       /**< [1] Boson Dart */
    {27, 28, 0, 29},      /**< [2] Slime Blower */
    {30, 0, 0, 29},       /**< [3] Slime Tether */
    {36, 37, 38, 39},     /**< [4] Stasis Stream */
    {40, 0, 41, 0},       /**< [5] Shock Blast */
    {46, 47, 48, 49},     /**< [6] Overload Pulse */
    {50, 0, 51, 0},       /**< [7] Meson Collider */
    {61, 62, 110, 111},   /**< [8] Red only snap */
    {61, 62, 110, 111},   /**< [9] Red only fade */
    {126, 127, 128, 129}, /**< [10] Afterlife */
};

/** @brief Alignment delay for wand lights per mode (ms). */
const uint16_t pack_sleep_align_ms[11] = {
    1100, /**< [0] Proton Stream */
    0,    /**< [1] Boson Dart */
    0,    /**< [2] Slime Blower */
    0,    /**< [3] Slime Tether */
    1600, /**< [4] Stasis Stream */
    0,    /**< [5] Shock Blast */
    1800, /**< [6] Overload Pulse */
    0,    /**< [7] Meson Collider */
    150,  /**< [8] Red only snap */
    300,  /**< [9] Red only fade */
    300   /**< [10] Afterlife */
};

/** @brief Timing configuration for monster sounds (in seconds). */
const MonsterTiming pack_monster_timing = {3, 30, 120};

/** @brief Mapping between monster sounds and response quotes. */
const uint8_t pack_monster_sound_pairs[16][2] = {
    {63, 81}, {64, 80}, {65, 78}, {66, 79}, {67, 80}, {68, 82},
    {69, 84}, {70, 83}, {71, 85}, {72, 87}, {73, 86},
    {74, 88}, {75, 82}, {76, 81}, {76, 89}, {77, 85},
};

/** @brief Total monster sound/response pairs available. */
const uint8_t pack_monster_sound_pair_count = 16;

/** @brief Number of slime blower related movie quotes. */
const uint8_t pack_slime_quote_count = 4;

/** @brief GPIO pin used for the sound module BUSY signal. */
const uint8_t pack_sound_busy_pin = 2;

/** @brief Logic level indicating the sound module is busy. */
const uint8_t pack_sound_busy_level = 0;

/** @brief UART baud rate for the external sound module. */
const uint32_t pack_sound_baud_rate = 9600;

/** @brief Maximum volume level accepted by the sound module. */
const uint8_t pack_sound_max_volume = 30;

/** @brief Repeating timer interval in milliseconds. */
const uint32_t pack_isr_interval_ms = 4;
