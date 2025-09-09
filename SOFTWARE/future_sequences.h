/**
 * @file future_sequences.h
 * @brief Manages animations for the "Future" (N-Filter) LED strip.
 * @details This file has been refactored. The animation logic is now in
 *          SOFTWARE/animations.h and SOFTWARE/animations.cpp.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef FUTURE_SEQUENCES_H
#define FUTURE_SEQUENCES_H

#include <stdbool.h>
#include <stdint.h>
#include "addressable_LED_support.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile CRGB future_color;

#ifdef __cplusplus
}
#endif

#endif // FUTURE_SEQUENCES_H
