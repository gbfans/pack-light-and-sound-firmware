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
#include "pack_state.h"
#include <FastLED.h>
#include <string.h>

// These are still needed for now, as they are used by the RotateAnimation
// and other parts of the code.
volatile CRGB cyclotron_after_set[3][3];
volatile CRGB cyclotron_color;
volatile uint8_t cyclotron_seq_num = 0;
volatile uint8_t cyclotron_color_set_size = 1;
volatile CRGB cyclotron_color_set[5];
volatile uint8_t g_cyclotron_led_count = NUM_LEDS_CYCLOTRON;

const uint8_t cyc_classic_pos[4][5] = {
    {  1,  2,  3,  4,  4}, // 4 lights only
    {  4, 10, 14, 20, 24}, // 24 lights only
    {  5, 13, 19, 27, 32}, // 32 lights only
    {  6, 16, 24, 34, 40}  // 40 lights only
};
