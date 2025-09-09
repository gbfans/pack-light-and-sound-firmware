/**
 * @file party_sequences.h
 * @brief Provides the interface for "Party Mode" animations.
 * @details This file defines the different party mode animations and provides
 *          functions to start, stop, and run them.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PARTY_SEQUENCES_H
#define PARTY_SEQUENCES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <FastLED.h>

/**
 * @brief Shared state for party mode animations.
 * @details This struct holds the state that is shared across all three
 *          LED strips during party mode animations.
 */
struct PartyModeState {
    // Shared state for Rainbow Fade
    uint8_t rainbow_hue = 0;

    // Shared state for Cylon Scanner
    CRGB cylon_eye_color = CRGB::Red;

    // Shared state for Random Sparkle
    int8_t sparkle_strip_index = -1; // -1 means no sparkle
    CRGB sparkle_color = CRGB::Black;
    uint32_t sparkle_time = 0;

    // Shared state for Beat Meter
    int16_t beat_meter_level = 0;
    int8_t beat_meter_direction = 1;
    CRGB beat_meter_color = CRGB::Black;
    uint8_t beat_meter_counter = 0;
    uint8_t beat_meter_max_level = 0;
};
#endif


/**
 * @brief Enumeration of the available party mode animations.
 */
typedef enum {
    PARTY_ANIMATION_RAINBOW_FADE,   /**< All LEDs fade through the color spectrum. */
    PARTY_ANIMATION_CYLON_SCANNER,  /**< A Cylon-style eye scans back and forth. */
    PARTY_ANIMATION_RANDOM_SPARKLE, /**< Random LEDs sparkle with random colors. */
    PARTY_ANIMATION_BEAT_METER,     /**< All strips act as a VU meter. */
    PARTY_ANIMATION_COUNT           /**< The total number of animations. */
} party_animation_t;

/**
 * @brief Runs the currently selected party mode animation.
 * @details This function should be called on every main loop iteration. It
 *          dispatches to the appropriate animation function.
 */
void party_mode_run(void);

/**
 * @brief Sets the active party mode animation.
 * @details Stops any currently running animations and starts the new one.
 * @param animation The animation to activate.
 */
void party_mode_set_animation(party_animation_t animation);

/**
 * @brief Stops the currently active party mode animation.
 * @details Clears all LEDs to black and disables party mode.
 */
void party_mode_stop(void);

/**
 * @brief Checks if party mode is currently active.
 * @return True if party mode is active, false otherwise.
 */
bool party_mode_is_active(void);

#endif // PARTY_SEQUENCES_H
