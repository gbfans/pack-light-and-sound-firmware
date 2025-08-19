/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack_config.h"
#include "addressable_LED_support.h"
#include "pack_state.h"
#include "colors.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"

/** Heat settings for each pack type: {start_beep, start_autovent, cool_factor}.
 */
const HeatSetting pack_heat_settings[5] = {
    {6 * 250, 10 * 250, 1}, /**< 4x Cyclotron Red only snap */
    {7 * 250, 11 * 250, 1}, /**< 4x Cyclotron Red only fade */
    {8 * 250, 13 * 250, 1}, /**< 4x Cyclotron TVG fade */
    {7 * 250, 11 * 250, 1}, /**< Afterlife Red */
    {8 * 250, 13 * 250, 1}, /**< Afterlife TVG */
};

/** Sound index for the short power-up by pack type. */
const uint8_t pack_short_powerup_sounds[5] = {93, 94, 94, 124, 124};

/** Powerdown sequences for each pack type. */
const PackSequence pack_powerdown_sequences[5] = {
    [PACK_TYPE_SNAP_RED] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_FADE_OUT, 2900},
    [PACK_TYPE_FADE_RED] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_FADE_OUT, 2900},
    [PACK_TYPE_TVG_FADE] = {59, PC_PATTERN_SHUTDOWN, 3100, CY_PATTERN_FADE_OUT, 3100},
    [PACK_TYPE_AFTERLIFE] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_RING_FADE_OUT, 2900},
    [PACK_TYPE_AFTER_TVG] = {11, PC_PATTERN_SHUTDOWN, 2900, CY_PATTERN_RING_FADE_OUT, 2900},
};

/** Number of selectable songs available via the song switch. */
const uint8_t pack_song_count = 3;

/** Minimum ADJ-derived cycle time in milliseconds. */
const uint16_t pack_adj_min_ms = 400;

/** Maximum ADJ-derived cycle time in milliseconds. */
const uint16_t pack_adj_max_ms = 1300;

/** Color selections for each pack mode. */
const PackModeColor pack_mode_colors[8] = {
    [PACK_MODE_PROTON_STREAM] = {COLOR_BLUE, 0, COLOR_WHITE},
    [PACK_MODE_BOSON_DART] = {COLOR_BLUE, 0, COLOR_WHITE},
    [PACK_MODE_SLIME_BLOWER] = {COLOR_GREEN, 1, COLOR_WHITE},
    [PACK_MODE_SLIME_TETHER] = {COLOR_GREEN, 1, COLOR_WHITE},
    [PACK_MODE_STASIS_STREAM] = {COLOR_BLUE, 2, COLOR_WHITE},
    [PACK_MODE_SHOCK_BLAST] = {COLOR_BLUE, 2, COLOR_WHITE},
    [PACK_MODE_OVERLOAD_PULSE] = {COLOR_ORANGE, 3, COLOR_WHITE},
    [PACK_MODE_MESON_COLLIDER] = {COLOR_ORANGE, 3, COLOR_WHITE},
};

/** Fire sound numbers for each mode and variant. */
const FireSoundSet pack_fire_sounds[11] = {
    {19, 16, 20, 18},     /**< Proton Stream */
    {21, 0, 22, 0},       /**< Boson Dart */
    {27, 28, 0, 29},      /**< Slime Blower */
    {30, 0, 0, 29},       /**< Slime Tether */
    {36, 37, 38, 39},     /**< Stasis Stream */
    {40, 0, 41, 0},       /**< Shock Blasty */
    {46, 47, 48, 49},     /**< Overload Pulse */
    {50, 0, 51, 0},       /**< Meson Collider */
    {61, 62, 110, 111},   /**< 8 - Red only snap */
    {61, 62, 110, 111},   /**< 9 - Red only fade */
    {126, 127, 128, 129}, /**< 10 - Afterlife */
};

/** Alignment delay for wand lights per mode (ms). */
const uint16_t pack_sleep_align_ms[11] = {
    1100, /**< Proton Stream */
    0,    /**< Boson Dart */
    0,    /**< Slime Blower */
    0,    /**< Slime Tether */
    1600, /**< Stasis Stream */
    0,    /**< Shock Blasty */
    1800, /**< Overload Pulse */
    0,    /**< Meson Collider */
    150,  /**< 8 - Red only snap */
    300,  /**< 9 - Red only fade */
    300   /**< 10 - Afterlife */
};

/** Timing configuration for monster sounds (in seconds). */
const MonsterTiming pack_monster_timing = {3, 30, 120};

/** Mapping between monster sounds and response quotes. */
const uint8_t pack_monster_sound_pairs[16][2] = {
    {63, 81}, {64, 80}, {65, 78}, {66, 79}, {67, 80}, {68, 82},
    {69, 84}, {70, 83}, {71, 85}, {72, 87}, {73, 86},
    {74, 88}, {75, 82}, {76, 81}, {76, 89}, {77, 85},
};

/** Total monster sound/response pairs available. */
const uint8_t pack_monster_sound_pair_count = 16;

/** Number of slime blower related movie quotes. */
const uint8_t pack_slime_quote_count = 4;

/** GPIO pin used for the sound module BUSY signal. */
const uint8_t pack_sound_busy_pin = 2;

/** Logic level indicating the sound module is busy. */
const uint8_t pack_sound_busy_level = 0;

/** UART baud rate for the external sound module. */
const uint32_t pack_sound_baud_rate = 9600;

/** Maximum volume level accepted by the sound module. */
const uint8_t pack_sound_max_volume = 30;

/** Repeating timer interval in milliseconds. */
const uint32_t pack_isr_interval_ms = 4;
