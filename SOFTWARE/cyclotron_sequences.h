/**
 * @file cyclotron_sequences.h
 * @brief Manages animations for the Cyclotron LED ring.
 * @details This file has been refactored. The animation logic is now in
 *          SOFTWARE/animations.h and SOFTWARE/animations.cpp.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef CYCLOTRON_SEQUENCES_H
#define CYCLOTRON_SEQUENCES_H

#include <stdbool.h>
#include <stdint.h>
#include "addressable_LED_support.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shared state read by the cyclotron animations in animations.cpp.
extern volatile CRGB cyclotron_color;
extern volatile uint8_t cyclotron_color_set_size;
extern volatile CRGB cyclotron_color_set[5];
extern volatile uint8_t g_cyclotron_led_count;

extern const uint8_t cyc_classic_pos[4][4];

#ifdef __cplusplus
}
static inline uint32_t afterlife_target_speed_x() {
    return (static_cast<uint32_t>(g_cyclotron_led_count) * 125U) / 40U;
}
#endif

#endif // CYCLOTRON_SEQUENCES_H
