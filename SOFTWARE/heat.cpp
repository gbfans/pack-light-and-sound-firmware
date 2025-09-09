/**
 * @file heat.cpp
 * @brief Implements the pack's heat simulation logic.
 * @details This file contains the core logic for the heat simulation, including
 *          the ISR-based temperature update function and the instant cooling
 *          function.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "heat.h"
#include "klystron_IO_support.h"
#include "pack_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global flag indicating if the pack is currently in a firing state. */
volatile bool firing_now = false;
/** @brief Global variable representing the current heat level of the pack. */
volatile uint16_t temperature = 0;

/**
 * @brief Updates the pack's temperature based on current state.
 * @details This function should be called from a repeating timer (ISR). It
 *          increments the temperature when `firing_now` is true and decrements
 *          it otherwise, simulating heating and cooling.
 * @note The rates of heating and cooling are defined internally in `heat.cpp`.
 */
void heat_isr(void) {
    if (firing_now) {
        temperature++;
    } else {
        const HeatSetting *setting = &pack_heat_settings[config_pack_type()];
        temperature = (temperature > setting->cool_factor)
                          ? temperature - setting->cool_factor
                          : 0;
    }
}

/**
 * @brief Resets the pack's temperature to zero.
 * @details This is typically called to simulate a full vent or when the pack
 *          is powered down, instantly cooling all components.
 */
void cool_the_pack(void) { temperature = 0; }

#ifdef __cplusplus
}
#endif
