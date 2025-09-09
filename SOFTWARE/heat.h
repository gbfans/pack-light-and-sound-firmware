/**
 * @file heat.h
 * @brief Manages the pack's heat simulation.
 * @details This file provides the interface for a simple heat simulation where
 *          firing increases the temperature and venting or inactivity cools it
 *          down. The temperature is used to trigger overheat effects.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef HEAT_H
#define HEAT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global flag indicating if the pack is currently in a firing state. */
extern volatile bool firing_now;

/** @brief Global variable representing the current heat level of the pack. */
extern volatile uint16_t temperature;

/**
 * @brief Updates the pack's temperature based on current state.
 * @details This function should be called from a repeating timer (ISR). It
 *          increments the temperature when `firing_now` is true and decrements
 *          it otherwise, simulating heating and cooling.
 * @note The rates of heating and cooling are defined internally in `heat.cpp`.
 */
void heat_isr(void);

/**
 * @brief Resets the pack's temperature to zero.
 * @details This is typically called to simulate a full vent or when the pack
 *          is powered down, instantly cooling all components.
 */
void cool_the_pack(void);

#ifdef __cplusplus
}
#endif

#endif // HEAT_H
