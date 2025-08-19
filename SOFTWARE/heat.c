/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "heat.h"
#include "klystron_IO_support.h"
#include "pack_config.h"

/** Indicates if the pack is currently firing. */
volatile bool firing_now = false;
/** Current heat level accumulated by firing or venting. */
volatile uint16_t temperature = 0;

/**
 * @brief Update the pack's temperature each timer tick.
 *
 * Increases heat while firing and gradually cools otherwise using the
 * configured cooling factor for the active pack type.
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
 * @brief Immediately reset the pack temperature to zero.
 */
void cool_the_pack(void) { temperature = 0; }

