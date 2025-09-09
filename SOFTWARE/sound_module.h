/**
 * @file sound_module.h
 * @brief Low-level driver for the serial sound board.
 * @details This file provides the direct interface for controlling an external
 *          serial sound module (like a DFPlayer Mini). It handles sending
 *          commands for playing, stopping, and managing volume.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef SOUND_MODULE_H
#define SOUND_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the serial interface to the sound module.
 * @details Sets up the UART communication and resets the sound module.
 */
void sound_init(void);

/**
 * @brief Starts playback of a sound by its index number.
 * @param sound_index The 1-based index of the sound file to play.
 */
void sound_start(uint8_t sound_index);

/**
 * @brief Waits until the current sound finishes playing.
 * @details This is a blocking function that polls the sound module's busy
 *          status. It can be configured to abort early if the user triggers
 *          a primary activation or shutdown event.
 * @param fire If true, the wait will abort if the main activation switch is pressed.
 * @param shutdown If true, the wait will abort if a pack shutdown is requested.
 */
void sound_wait_til_end(bool fire, bool shutdown);

/**
 * @brief Checks if the sound module is currently playing a sound.
 * @details This is determined by reading the state of the sound module's
 *          `BUSY` pin.
 * @return true if audio is playing, false otherwise.
 */
bool sound_is_playing(void);

/**
 * @brief Stops the currently playing sound immediately.
 */
void sound_stop(void);

/**
 * @brief Pauses playback of the current sound.
 * @note The sound can be resumed from the same position with `sound_resume()`.
 */
void sound_pause(void);

/**
 * @brief Resumes playback of a previously paused sound.
 */
void sound_resume(void);

/**
 * @brief Plays a sound in a continuous loop.
 * @param sound_index The index of the sound file to repeat.
 */
void sound_repeat(uint8_t sound_index);

/**
 * @brief Sets the playback volume level.
 * @param volume_level The new volume level, typically from 0 (min) to 30 (max).
 */
void sound_volume(uint8_t volume_level);

#ifdef __cplusplus
}
#endif

#endif // SOUND_MODULE_H
