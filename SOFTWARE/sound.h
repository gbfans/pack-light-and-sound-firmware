/**
 * @file sound.h
 * @brief High-level sound event management.
 * @details This file provides the interface for playing sounds that are
 *          closely tied to the pack's state and mode, such as the main
 *          activation (firing) sounds.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manages the sound effects for the main activation sequence.
 * @details Plays a sound associated with the current pack's main activation
 *          (e.g., firing, overheat). The specific sound played is determined
 *          by the current `PackMode` and the `fire_type` index.
 * @param fire_type An index indicating the type of event:
 *                  - 0: Start/continue activation sound.
 *                  - 1: End activation sound.
 *                  - 2: Overheat warning sound during activation.
 *                  - 3: Stop sound due to overheat.
 */
void fire_department(uint8_t fire_type);

/**
 * @brief Delays execution to align wand lights with sound during overheat.
 * @details This is a blocking delay whose duration is determined by the
 *          current pack mode. It's used to synchronize visual effects with
 *          specific sound cues.
 */
void sleep_align_wandlights(void);

/**
 * @brief Initializes the sound subsystem.
 * @details This function should be called once at startup to initialize the
 *          sound module and unmute the amplifier.
 */
void sound_startup(void);

#ifdef __cplusplus
}
#endif

#endif // SOUND_H
