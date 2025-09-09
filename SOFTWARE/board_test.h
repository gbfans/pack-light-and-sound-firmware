/**
 * @file board_test.h
 * @brief Provides an interface for the hardware board test routine.
 * @details This routine is used for manufacturing and diagnostics to verify
 *          that all hardware components (LEDs, switches, sound) are
 *          functioning correctly.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef BOARD_TEST_H
#define BOARD_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runs the main board test sequence.
 * @details This is a blocking function that cycles through a series of tests:
 *          - Lights up all LEDs in different colors.
 *          - Plays test sounds.
 *          - Waits for the user to toggle each of the switches.
 *          The user advances through the test steps by pressing the fire button.
 */
void board_test(void);

#ifdef __cplusplus
}
#endif

#endif // BOARD_TEST_H
