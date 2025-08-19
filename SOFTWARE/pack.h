/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_H
#define PACK_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Execute startup routine for the active pack type.
 */
void pack_combo_startup(void);

/**
 * @brief Play a short powerup sound.
 *
 * @param afterlife_higher Use alternate sound for higher afterlife tone.
 */
void pack_short_powerup_sound(bool afterlife_higher);

/**
 * @brief Execute powerdown routine for the active pack type.
 */
void pack_combo_powerdown(void);

#endif // PACK_H

