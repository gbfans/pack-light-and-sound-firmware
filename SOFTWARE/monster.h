/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef MONSTER_H
#define MONSTER_H

#include <stdint.h>
#include <stdbool.h>

/** Monster countdown timer in 4 ms ticks. */
extern volatile uint32_t monster_timer;
/** Response countdown timer in 4 ms ticks. */
extern volatile uint32_t response_timer;

/**
 * @brief Update monster and response timers every 4 ms tick.
 */
void monster_isr(void);

/**
 * @brief Register a fire event to trigger a monster response.
 */
void monster_fire(void);

/**
 * @brief Reset all monster timers.
 */
void monster_clear(void);

#endif // MONSTER_H
