/**
 * @file cyclotron_sequences.cpp
 * @brief Cyclotron ring control.
 * @details This file has been refactored. The animation logic is now in
 *          SOFTWARE/animations.cpp.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "cyclotron_sequences.h"
#include "addressable_LED_support.h"
#include "build_options.h"
#include "pack_state.h"
#include <FastLED.h>
#include <string.h>

// Shared state read by the cyclotron animations in animations.cpp.
volatile CRGB cyclotron_color;
volatile uint8_t cyclotron_color_set_size = 1;
volatile CRGB cyclotron_color_set[5];
#if POTS_DISABLED
// Fixed-ring build: start at the compiled-in size so the very first frame is
// already correct, and never let ADJ1 move it (see ring_monitor()).
volatile uint8_t g_cyclotron_led_count = STATIC_CYCLOTRON_LED_COUNT;
#else
volatile uint8_t g_cyclotron_led_count = NUM_LEDS_CYCLOTRON;
#endif

/** 1-based LED positions of the four classic cyclotron windows for each
 *  supported ring size. */
const uint8_t cyc_classic_pos[4][4] = {
    {  1,  2,  3,  4}, // 4 lights only
    {  4, 10, 14, 20}, // 24 lights only
    {  5, 13, 19, 27}, // 32 lights only
    {  6, 16, 24, 34}  // 40 lights only
};
