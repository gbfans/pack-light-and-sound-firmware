/**
 * @file pack.h
 * @brief Top-level pack behavior functions.
 * @details This file provides the main entry points for complex, coordinated
 *          pack behaviors like startup and shutdown sequences that involve
 *          multiple sounds and lighting effects.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_H
#define PACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Executes the main power-up sequence for the currently active pack type.
 * @details This function coordinates the sound and light animations for a full
 *          pack startup. The specific sounds and light patterns are determined
 *          by the pack type selected via DIP switches.
 */
void pack_combo_startup(void);

/**
 * @brief Plays a short power-up sound, typically for state transitions.
 * @details This is used when the pack is already partially on and is
 *          transitioning to a more active state, such as from wand standby
 *          to full idle.
 * @param afterlife_higher For Afterlife packs, if true, plays a higher-pitched
 *                         power-up tone. This parameter has no effect on other
 *                         pack types.
 */
void pack_short_powerup_sound(bool afterlife_higher);

/**
 * @brief Executes the main power-down sequence for the currently active pack type.
 * @details This function coordinates the sound and light animations for a full
 *          pack shutdown.
 */
void pack_combo_powerdown(void);

/**
 * @brief Manages the various firing sounds and effects.
 * @param fire_type An index indicating the type of firing event:
 *                  0 for starting/continuing firing, 1 for ending firing,
 *                  2 for overheat warning, 3 for stopping due to overheat.
 */
void fire_department(uint8_t fire_type);

#ifdef __cplusplus
}
#endif

#endif // PACK_H
