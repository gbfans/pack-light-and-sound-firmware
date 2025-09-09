/**
 * @file powercell_sequences.cpp
 * @brief Manages the animation patterns for the Powercell LEDs.
 * @details This file has been refactored. The animation logic is now in
 *          SOFTWARE/animations.h and SOFTWARE/animations.cpp.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "powercell_sequences.h"
#include "addressable_LED_support.h"

// The FastLED library is used for the addressable LEDs
#include <FastLED.h>
#include <string.h>

volatile CRGB powercell_color; // Set during pack color updates
// This is still needed for now, as it's set by update_pack_colors
// and read in pack.cpp. I will remove it in a later step.
