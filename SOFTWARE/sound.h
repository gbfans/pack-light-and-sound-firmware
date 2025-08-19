/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

/**
 * @brief Play a fire-related sound for the current pack mode.
 *
 * @param sound_index Index into the fire sound set.
 */
void fire_department(uint8_t sound_index);

/**
 * @brief Delay to align wand lights during overheat sequences.
 */
void sleep_align_wandlights(void);

/**
 * @brief Initialize the sound subsystem and amplifier.
 */
void sound_startup(void);

#endif // SOUND_H
