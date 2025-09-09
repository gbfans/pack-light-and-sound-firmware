/**
 * @file future_sequences.cpp
 * @brief Manages animations for the "Future" (N-Filter) LED strip.
 * @details This file has been refactored. The animation logic is now in
 *          SOFTWARE/animations.cpp.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "future_sequences.h"
#include "addressable_LED_support.h"
#include <FastLED.h>

volatile CRGB future_color; // Set during pack color updates
// This is still needed for now, as it's set by update_pack_colors
// and read in monitors.cpp. I will remove it in a later step.
