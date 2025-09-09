/**
 * @file addressable_LED_support.h
 * @brief Low-level driver interface for addressable LED strips.
 * @details This file defines the hardware configuration (pin numbers, LED counts)
 *          and provides the main interface for initializing and updating the
 *          physical LED strips.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef ADDRESSABLE_LED_SUPPORT_H
#define ADDRESSABLE_LED_SUPPORT_H

#include <FastLED.h>

// === Hardware Definitions ===

/** @brief The number of LEDs in the Powercell strip. */
#define NUM_LEDS_POWERCELL 15
/** @brief The physical number of LEDs in the Cyclotron ring. */
#define NUM_LEDS_CYCLOTRON 40
/** @brief The number of LEDs in the "Future" (N-Filter) strip. */
#define NUM_LEDS_FUTURE 16

/** @brief The GPIO data pin for the Powercell LED strip. */
#define POWERCELL_PIN 3
/** @brief The GPIO data pin for the Cyclotron LED strip. */
#define CYCLOTRON_PIN 4
/** @brief The GPIO data pin for the "Future" (N-Filter) LED strip. */
#define FUTURE_PIN 5

// === Global LED Buffers ===

/** @brief The FastLED buffer for the Powercell LEDs. */
extern CRGB g_powercell_leds[NUM_LEDS_POWERCELL];
/** @brief The FastLED buffer for the Cyclotron LEDs. */
extern CRGB g_cyclotron_leds[NUM_LEDS_CYCLOTRON];
/** @brief The FastLED buffer for the "Future" (N-Filter) LEDs. */
extern CRGB g_future_leds[NUM_LEDS_FUTURE];

#ifdef __cplusplus
extern "C" {
#endif

// === Function Prototypes ===

/**
 * @brief Initializes all LED strips via the FastLED library.
 * @details Configures the controller, pin, and color order for each of the
 *          three physical LED strips.
 */
void init_leds();

/**
 * @brief Pushes the current state of all LED buffers to the physical strips.
 * @details This function is the single point of truth for updating the hardware.
 *          It applies the brightness ramp and the cyclotron LED mask before
 *          calling `FastLED.show()`.
 */
void show_leds();

/**
 * @brief Sets the target brightness for all LEDs, ramping over a duration.
 * @param brightness The target brightness (0-255).
 * @param duration The time in milliseconds for the brightness transition.
 */
void set_led_brightness(uint8_t brightness, unsigned long duration);

/**
 * @brief Masks off unused cyclotron LEDs so the remainder stays dark.
 */
void mask_cyclotron_leds();

#ifdef __cplusplus
}
#endif

#endif // ADDRESSABLE_LED_SUPPORT_H
