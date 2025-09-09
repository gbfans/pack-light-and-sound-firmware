/**
 * @file monitors.h
 * @brief Interface for high-level monitors for inputs and events.
 * @details This file declares the monitor functions that are called on each
 *          main loop iteration to check for input changes (switches, pots),
 *          and manage ongoing events like songs, hum, and the monster mode.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef MONITORS_H
#define MONITORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define PC_SPEED_DEFAULT 0

/** @brief Global state for song playback; MSB is set when a song is playing. */
extern volatile uint8_t song;


/** @brief Checks if a song is currently playing. */
bool song_is_playing(void);

/** @brief Monitors the song switch for changes and handles song/party mode logic. */
void song_monitor(void);

/** @brief Starts a sound safely, ensuring any currently playing sound is stopped first. */
void sound_start_safely(uint8_t sound_index);

/**
 * @brief Plays a sound and blocks until it completes.
 * @param sound_index Index of the sound to start.
 * @param fire Whether the function should abort if firing is activated.
 * @param shutdown Whether the function should abort if shutdown is requested.
 */
void sound_play_blocking(uint8_t sound_index, bool fire, bool shutdown);

/** @brief Manages playback of the idle hum sound based on pack mode and DIP switch settings. */
void hum_monitor(void);

/** @brief Manages the interactive monster sound Easter egg. */
void monster_monitor(void);

/**
 * @brief Converts an ADJ potentiometer reading to a pattern cycle time in milliseconds.
 * @param adj_select Which potentiometer to read (0 or 1).
 * @param heat_effect If true, applies a speed adjustment based on pack temperature.
 * @param apply_cy_speed If true, ignores ADJ0 and applies the Afterlife
 *        cyclotron speed multiplier to a fixed midpoint speed.
 * @return Computed cycle duration in milliseconds.
 */
uint16_t adj_to_ms_cycle(uint8_t adj_select, bool heat_effect, bool apply_cy_speed);

/** @brief Polls the adjustment potentiometers and updates relevant animation speeds. */
void adj_monitor(void);

/** @brief Monitors for fire button taps to cycle through TVG weapon modes. */
void mode_monitor(void);

/** @brief Monitors the vent switch to trigger vent sequences or play quotes. */
void vent_monitor(void);

/** @brief Monitors the ADJ1 potentiometer to update the cyclotron's active LED count (`N`). */
void ring_monitor(void);

/** @brief Triggers a full vent sequence with coordinated lights and sound. */
void full_vent(void);

#ifdef __cplusplus
}
#endif

#endif // MONITORS_H
