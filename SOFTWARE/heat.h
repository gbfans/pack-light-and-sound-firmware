/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef HEAT_H
#define HEAT_H

#include <stdbool.h>
#include <stdint.h>

/** Indicates if the pack is currently firing. */
extern volatile bool firing_now;

/** Current heat level accumulated by firing or venting. */
extern volatile uint16_t temperature;

/**
 * @brief Update temperature each millisecond.
 */
void heat_isr(void);

/**
 * @brief Reset the pack temperature to a safe value.
 */
void cool_the_pack(void);

#endif // HEAT_H
