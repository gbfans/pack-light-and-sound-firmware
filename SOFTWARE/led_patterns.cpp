/**
 * @file led_patterns.cpp
 * @brief Implements global color assignments for LED animations.
 * @details This file provides the implementation for updating the global color
 *          variables used by the various LED sequence files.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "led_patterns.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "pack_state.h"
#include "pack_config.h"
#include "hardware/sync.h"
#include <string.h>

/**
 * @brief Updates the global color variables based on the active pack mode.
 * @details This function reads the current pack mode (e.g., Proton Stream,
 *          Slime Blower) from the pack state and sets the `powercell_color`,
 *          `cyclotron_color`, and `future_color` volatile variables from the
 *          `pack_mode_colors` configuration table. The animation sequences
 *          then use these global variables to render their patterns.
 * @note It contains special case logic to force the cyclotron color to red for
 *       Afterlife modes. It also re-initializes the Afterlife color set.
 * @post The global `..._color` variables are updated.
 */
void update_pack_colors(void) {
    PackMode mode = pack_state_get_mode();
    CRGB pc = pack_mode_colors[mode].powercell;
    CRGB cy = pack_mode_colors[mode].cyclotron;
    CRGB fr = pack_mode_colors[mode].future;
    if (config_pack_type() == PACK_TYPE_AFTERLIFE) {
        // Plain Afterlife packs keep a red ring regardless of mode; only
        // Afterlife TVG recolors the cyclotron per weapon.
        cy = CRGB::Red;
    }
    // The color globals are three bytes each and are read by the animations
    // running in the timer ISR; mask interrupts so a color change can never
    // be observed half-written (one frame with mixed channels).
    uint32_t irq_state = save_and_disable_interrupts();
    memcpy((void*)&powercell_color, &pc, sizeof(CRGB));
    memcpy((void*)&cyclotron_color, &cy, sizeof(CRGB));
    memcpy((void*)&future_color, &fr, sizeof(CRGB));
    restore_interrupts(irq_state);
}

