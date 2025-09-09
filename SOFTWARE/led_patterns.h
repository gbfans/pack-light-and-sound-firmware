/**
 * @file led_patterns.h
 * @brief Manages global color assignments for LED animations.
 * @details This file provides the interface for updating the global color
 *          variables used by the various LED sequence files.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Updates the global color variables based on the active pack mode.
 * @details This function reads the current pack mode (e.g., Proton Stream,
 *          Slime Blower) and sets the `powercell_color`, `cyclotron_color`,
 *          and `future_color` volatile variables. The animation sequences
 *          then use these global variables to render their patterns.
 * @post The global `..._color` variables are updated.
 */
void update_pack_colors(void);

#ifdef __cplusplus
}
#endif

#endif // LED_PATTERNS_H
