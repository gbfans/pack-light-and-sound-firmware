/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

/**
 * @file monster.c
 * @brief Timer utilities for interactive monster sounds.
 */

#include "monster.h"

/** Monster countdown timer in 4 ms ticks. */
volatile uint32_t monster_timer = 0;
/** Response countdown timer in 4 ms ticks. */
volatile uint32_t response_timer = 0;

/**
 * @brief Maintain monster and response timers.
 *
 * Called each tick to decrement the monster countdown and response window
 * timers, handling timeout conditions when appropriate.
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
 * @brief Trigger a monster response if the firing window is active.
 */
void monster_fire(void) {
    if (monster_timer == 2) {
        monster_timer = 1;
    }
}

/**
 * @brief Reset all monster timers.
 */
void monster_clear(void) {
    monster_timer = 0;
    response_timer = 0;
}

