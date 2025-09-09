/**
 * @file monster.cpp
 * @brief Implements the "Monster" sound Easter egg logic.
 * @details This file contains the core logic for the interactive monster sound
 *          mode, including the ISR-based timer updates and state changes.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "monster.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Countdown timer for the next monster sound event.
 * @details This timer is decremented by the ISR. When it reaches zero, a new
 *          monster sound is triggered by `monster_monitor`. Measured in 4ms ticks.
 */
volatile uint32_t monster_timer = 0;
/**
 * @brief Countdown timer for the user to fire and get a response.
 * @details After a monster sound plays, this timer starts. If `monster_fire()`
 *          is called before it expires, a response sound is played. Measured in 4ms ticks.
 */
volatile uint32_t response_timer = 0;

/**
 * @brief Updates the monster and response timers.
 * @details This function should be called from a repeating timer (ISR). It
 *          decrements the active timers, which control the timing of the
 *          monster sound events.
 */
void monster_isr(void) {
    if (monster_timer > 3) {
        response_timer = 0;
        monster_timer--;
    } else if ((monster_timer == 2) && (response_timer == 0)) {
        monster_timer = 0;
    } else if (response_timer > 0) {
        response_timer--;
    }
}

/**
 * @brief Registers a fire event for a potential monster response.
 * @details If this function is called while the `response_timer` is active
 *          (indicated by `monster_timer == 2`), it sets `monster_timer` to 1,
 *          which signals the `monster_monitor` to play the response sound.
 */
void monster_fire(void) {
    if (monster_timer == 2) {
        monster_timer = 1;
    }
}

/**
 * @brief Resets all monster mode timers and states.
 * @details This is called to disable the monster mode or reset it to a clean state.
 */
void monster_clear(void) {
    monster_timer = 0;
    response_timer = 0;
}

#ifdef __cplusplus
}
#endif
