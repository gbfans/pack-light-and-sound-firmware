/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef MONITORS_H
#define MONITORS_H

#include <stdbool.h>
#include <stdint.h>

/** Track song state; MSB set when a song is playing. */
extern volatile uint8_t song;

/** Check if a song is currently playing. */
bool song_is_playing(void);

/** Monitor the song switch and handle playback changes. */
void song_monitor(void);

/** Start a sound and ensure playback begins. */
void sound_start_safely(uint8_t sound_index);

/**
 * @brief Play a sound and wait until completion.
 *
 * @param sound_index Index of the sound to start.
 * @param fire        Whether firing is active.
 * @param shutdown    Whether shutdown is requested.
 */
void sound_play_blocking(uint8_t sound_index, bool fire, bool shutdown);

/** Maintain hum playback based on pack mode. */
void hum_monitor(void);

/** Handle random monster sounds and responses. */
void monster_monitor(void);

/**
 * @brief Convert ADJ potentiometer to timing.
 *
 * @param adj_select Which potentiometer to read.
 * @param heat_effect Apply heat scaling if true.
 * @param apply_cy_speed Apply cyclotron speed multiplier when true.
 * @return Duration in milliseconds.
 */
uint16_t adj_to_ms_cycle(uint8_t adj_select, bool heat_effect, bool apply_cy_speed);

/** Poll the adjustment potentiometers. */
void adj_monitor(void);

/** Monitor for mode changes triggered by fire taps. */
void mode_monitor(void);

/** Monitor vent switch and trigger sequences. */
void vent_monitor(void);

/** Update cyclotron ring count from ADJ1. */
void ring_monitor(void);

/** Save the current state of pack light patterns. */
void save_pack_light_states(void);

/** Restore previously saved pack light patterns. */
void restore_pack_light_states(void);

/** Trigger a full vent sequence. */
void full_vent(void);

#endif // MONITORS_H
