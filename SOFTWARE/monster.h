/**
 * @file monster.h
 * @brief Manages the "Monster" sound Easter egg.
 * @details This file provides the interface for the interactive monster sound
 *          mode. When enabled, the pack will play a random "monster" sound.
 *          If the user fires the pack shortly after, a "response" sound will
 *          play.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef MONSTER_H
#define MONSTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Countdown timer for the next monster sound event.
 * @details This timer is decremented by the ISR. When it reaches zero, a new
 *          monster sound is triggered by `monster_monitor`. Measured in 4ms ticks.
 */
extern volatile uint32_t monster_timer;

/**
 * @brief Countdown timer for the user to fire and get a response.
 * @details After a monster sound plays, this timer starts. If `monster_fire()`
 *          is called before it expires, a response sound is played. Measured in 4ms ticks.
 */
extern volatile uint32_t response_timer;

/**
 * @brief Updates the monster and response timers.
 * @details This function should be called from a repeating timer (ISR). It
 *          decrements the active timers.
 */
void monster_isr(void);

/**
 * @brief Registers a fire event for a potential monster response.
 * @details If the `response_timer` is active, this function will trigger the
 *          monster response sound.
 */
void monster_fire(void);

/**
 * @brief Resets all monster mode timers and states.
 * @details This is called to disable the monster mode or reset it to a clean state.
 */
void monster_clear(void);

#ifdef __cplusplus
}
#endif

#endif // MONSTER_H
