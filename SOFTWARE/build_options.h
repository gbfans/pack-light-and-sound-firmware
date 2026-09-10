/**
 * @file build_options.h
 * @brief Compile-time build variants for the klystron firmware.
 * @details The stock firmware reads two adjustment potentiometers: ADJ0 sets
 *          the animation speed and ADJ1 selects the cyclotron ring size (`N`).
 *          Those pots are a common field failure, and a broken pot leaves the
 *          pack stuck at whatever value the dead wiper happens to read.
 *
 *          Defining `STATIC_CYCLOTRON_LED_COUNT` builds a variant that never
 *          touches the ADC: the ring size is fixed at compile time, the speed
 *          is fixed at the midpoint of the normal range, and neither can be
 *          changed from the hardware. See `SOFTWARE/CMakeLists.txt` for the
 *          variant targets that define it.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

#ifdef STATIC_CYCLOTRON_LED_COUNT

// Only the four ring sizes the animations have offset tables for are valid.
#if (STATIC_CYCLOTRON_LED_COUNT != 4) && (STATIC_CYCLOTRON_LED_COUNT != 24) && \
    (STATIC_CYCLOTRON_LED_COUNT != 32) && (STATIC_CYCLOTRON_LED_COUNT != 40)
#error "STATIC_CYCLOTRON_LED_COUNT must be one of 4, 24, 32 or 40"
#endif

/** @brief Non-zero when this build ignores the adjustment potentiometers. */
#define POTS_DISABLED 1

#else

#define POTS_DISABLED 0

#endif // STATIC_CYCLOTRON_LED_COUNT

#if POTS_DISABLED
/**
 * @brief Value reported for both ADJ inputs when the pots are disabled.
 * @details Midpoint of the 12-bit ADC range. Nothing in a pots-disabled build
 *          derives behavior from `adj_pot[]`, but the array stays at a sane
 *          value so any diagnostic that prints it reads mid-scale rather than
 *          a stuck-at-zero fault.
 */
#define POT_FIXED_READING 2048
#endif

#endif // BUILD_OPTIONS_H
