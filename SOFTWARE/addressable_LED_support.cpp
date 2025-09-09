/**
 * @file addressable_LED_support.cpp
 * @brief Low-level driver for addressable LED strips.
 * @details This file provides initialization and a centralized update function
 *          (`show_leds`) for all addressable LED strips (Powercell, Cyclotron, Future).
 *          It wraps the FastLED library calls and includes the crucial cyclotron
 *          LED masking logic.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "addressable_LED_support.h"
#include "Ramp.h"
#include "cyclotron_sequences.h"

// Define the CRGB arrays for each strip
CRGB g_powercell_leds[NUM_LEDS_POWERCELL];
CRGB g_cyclotron_leds[NUM_LEDS_CYCLOTRON];
CRGB g_future_leds[NUM_LEDS_FUTURE];

// Ramp object controlling global LED brightness
static rampByte g_brightness_ramp(255);

/**
 * @brief Initializes all LED strips via the FastLED library.
 * @details Configures the controller, pin, and color order for each of the
 *          three physical LED strips.
 */
void init_leds() {
  // Initialize the Powercell LEDs with explicit GRB color order
  FastLED.addLeds<WS2812B, POWERCELL_PIN, GRB>(g_powercell_leds, NUM_LEDS_POWERCELL);

  // Initialize the Cyclotron LEDs with explicit GRB color order to ensure
  // red renders correctly on all hardware revisions
  FastLED.addLeds<WS2812B, CYCLOTRON_PIN, GRB>(g_cyclotron_leds, NUM_LEDS_CYCLOTRON);

  // Initialize the Future LEDs with explicit GRB color order
  FastLED.addLeds<WS2812B, FUTURE_PIN, GRB>(g_future_leds, NUM_LEDS_FUTURE);

  // Start with full brightness
  FastLED.setBrightness(g_brightness_ramp.getValue());
}

/**
 * @brief Sets the target brightness for all LEDs, ramping over a duration.
 * @param brightness The target brightness (0-255).
 * @param duration The time in milliseconds for the brightness transition.
 */
void set_led_brightness(uint8_t brightness, unsigned long duration) {
  // Use a quadratic easing curve for smoother fades that start gently
  // and accelerate toward the target brightness.
  g_brightness_ramp.go(brightness, duration, QUADRATIC_INOUT);
}

/**
 * @brief Masks off unused cyclotron LEDs.
 * @details Ensures that all LEDs from the active count (N) to the physical
 *          total (40) are set to black, enforcing the "remainder off" rule.
 *          This is called by `show_leds` before updating the physical strips.
 */
void mask_cyclotron_leds() {
    // Clamp the active LED count to the physical strip size to guard
    // against accidental overruns that could corrupt adjacent memory.
    if (g_cyclotron_led_count > NUM_LEDS_CYCLOTRON) {
        g_cyclotron_led_count = NUM_LEDS_CYCLOTRON;
    }

    // Turn off all LEDs from the current count up to the physical max.
    // This guarantees that modes which operate on N LEDs don't leave
    // leftover pixels lit if N is reduced.
    for (int i = g_cyclotron_led_count; i < NUM_LEDS_CYCLOTRON; i++) {
        g_cyclotron_leds[i] = CRGB::Black;
    }
}

/**
 * @brief Pushes the current state of all LED buffers to the physical strips.
 * @details This function is the single point of truth for updating the hardware.
 *          It applies the brightness ramp and the cyclotron LED mask before
 *          calling `FastLED.show()`.
 */
void show_leds() {
  // Apply the cyclotron mask before showing the LEDs.
  mask_cyclotron_leds();

  FastLED.setBrightness(g_brightness_ramp.update());
  FastLED.show();
}
