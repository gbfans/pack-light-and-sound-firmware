/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef SOUND_MODULE_H
#define SOUND_MODULE_H

#include <stdbool.h>
#include <stdint.h>

/** Initialize the serial sound module. */
void sound_init(void);

/** Start playback of a sound by index. */
void sound_start(uint8_t sound_index);

/**
 * @brief Wait until playback finishes.
 *
 * @param fire Abort if the fire switch is pressed.
 * @param shutdown Abort if powerdown is requested.
 */
void sound_wait_til_end(bool fire, bool shutdown);

/**
 * @brief Determine if the sound module is currently busy.
 *
 * @return true if audio is playing.
 */
bool sound_is_playing(void);

/** Stop the currently playing sound. */
void sound_stop(void);

/** Pause the current sound. */
void sound_pause(void);

/** Resume a previously paused sound. */
void sound_resume(void);

/**
 * @brief Play a sound in a repeating loop.
 *
 * @param sound_index Index of the sound to repeat.
 */
void sound_repeat(uint8_t sound_index);

/**
 * @brief Set the playback volume level.
 *
 * @param volume_level New volume level (0-30).
 */
void sound_volume(uint8_t volume_level);

#endif // SOUND_MODULE_H

