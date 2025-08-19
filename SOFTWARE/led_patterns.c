/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "led_patterns.h"
#include "addressable_LED_support.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "pack_state.h"
#include "pack_config.h"

/**
 * @brief Refresh LED colors based on the current pack mode.
 */
void update_pack_colors(void) {
    PackMode mode = pack_state_get_mode();
    powercell_color = pack_mode_colors[mode].powercell;
    cy_color_set    = pack_mode_colors[mode].cyclotron_set;
    future_color    = pack_mode_colors[mode].future;
}

