/**
 * @file board_test.cpp
 * @brief Implements the hardware board test routine.
 * @details This routine is used for manufacturing and diagnostics to verify
 *          that all hardware components (LEDs, switches, sound) are
 *          functioning correctly.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "board_test.h"
#include "addressable_LED_support.h"
#include "klystron_IO_support.h"
#include "pico/stdlib.h"
#include "sound_module.h"

// Map digits 0-9 to sound indices.  The sound module uses
// tracks 1-9 for "1"-"9" and track 10 for "0".
static const uint8_t digit_sounds[10] = {0x0A, 0x01, 0x02, 0x03, 0x04,
                                        0x05, 0x06, 0x07, 0x08, 0x09};

/**
 * @brief Announce the current firmware version using sound and LEDs.
 * @param color LED color used for blink feedback.
 */
static void announce_version(CRGB color) {
    const uint8_t digits[3] = {FW_VERSION_MAJOR, FW_VERSION_MINOR,
                              FW_VERSION_PATCH};
    const int leds[3] = {NUM_LEDS_POWERCELL - 1, NUM_LEDS_POWERCELL / 2, 0};

    // Start with a "test" sound.
    sound_start(0x0);
    sound_wait_til_end(false, false);

    for (int part = 0; part < 3; ++part) {
        // Blink the corresponding powercell LED to indicate the digit value.
        for (uint8_t b = 0; b < digits[part]; ++b) {
            g_powercell_leds[leds[part]] = color;
            show_leds();
            sleep_ms(150);
            g_powercell_leds[leds[part]] = CRGB::Black;
            show_leds();
            sleep_ms(150);
        }

        // Speak the digit.
        sound_start(digit_sounds[digits[part] % 10]);
        sound_wait_til_end(false, false);

        if (part < 2) {
            // Click between major/minor/patch numbers.
            sound_start(0x0C);
            sound_wait_til_end(false, false);
        }
    }

    // End with a "test" sound and short pause.
    sound_start(0x0);
    sound_wait_til_end(false, false);
    sleep_ms(2000);
}

/**
 * @brief Helper function to fill all LED strips with a single color.
 * @param color The color to fill the strips with.
 * @param delay The delay in milliseconds to hold the color.
 */
static void test_color(CRGB color, int delay) {
    fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, color);
    show_leds();
    sleep_ms(delay);

    fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, color);
    show_leds();
    sleep_ms(delay);

    fill_solid(g_future_leds, NUM_LEDS_FUTURE, color);
    show_leds();
    sleep_ms(delay);
}

/**
 * @brief Runs the main board test sequence.
 * @details This is a blocking function that cycles through a series of tests:
 *          - Plays test sounds to verify the sound module.
 *          - Lights up all LEDs in different colors to check for dead pixels.
 *          - Displays potentiometer readings on the powercell.
 *          - Displays switch states on the powercell.
 *          - Toggles the vent light.
 *          The user advances through the test steps by pressing the fire button.
 * @note This function is intended for diagnostic purposes and is only run
 *       when the firmware is started in a special test mode.
 */
void board_test(void) {
  CRGB powercell_color = CRGB::Blue;
  announce_version(powercell_color);

  const int delay = 25;
  fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
  fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, CRGB::Black);
  fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
  show_leds();

  test_color(CRGB::Red, delay);
  test_color(CRGB::Green, delay);
  test_color(CRGB::Blue, delay);
  sleep_ms(1000);

  fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
  fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, CRGB::Black);
  fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
  g_powercell_leds[0] = CRGB::Red;
  g_powercell_leds[1] = CRGB::Green;
  g_powercell_leds[2] = CRGB::Blue;
  g_powercell_leds[NUM_LEDS_POWERCELL - 2] = CRGB::Red + CRGB::Blue;
  g_powercell_leds[NUM_LEDS_POWERCELL - 1] = CRGB::Red + CRGB::Green;
  g_cyclotron_leds[0] = CRGB::Red;
  g_cyclotron_leds[1] = CRGB::Green;
  g_cyclotron_leds[2] = CRGB::Blue;
  g_cyclotron_leds[NUM_LEDS_CYCLOTRON - 2] = CRGB::Red + CRGB::Blue;
  g_cyclotron_leds[NUM_LEDS_CYCLOTRON - 1] = CRGB::Red + CRGB::Green;
  g_future_leds[0] = CRGB::Red;
  g_future_leds[1] = CRGB::Green;
  g_future_leds[2] = CRGB::Blue;
  g_future_leds[NUM_LEDS_FUTURE - 2] = CRGB::Red + CRGB::Blue;
  g_future_leds[NUM_LEDS_FUTURE - 1] = CRGB::Red + CRGB::Green;
  show_leds();
  sleep_ms(5000);

  fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
  fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, CRGB::Black);
  fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
  show_leds();
  sleep_ms(5);

  sound_start(0x17);
  sound_wait_til_end(false, false);
  do {
    read_adj_potentiometers(true);
    g_powercell_leds[4] = (adj_pot[0] > 4050) ? CRGB::Green : CRGB::Black;
    g_powercell_leds[3] = ((adj_pot[0] > 2450) && (adj_pot[0] <= 4050)) ? CRGB(adj_pot[0] >> 4, 0, 0) : CRGB::Black;
    g_powercell_leds[2] = ((adj_pot[0] >= 1450) && (adj_pot[0] <= 2450)) ? CRGB(adj_pot[0] >> 4, 0, 0) : CRGB::Black;
    g_powercell_leds[1] = ((adj_pot[0] < 1450) && (adj_pot[0] >= 50)) ? CRGB(adj_pot[0] >> 4, 0, 0) : CRGB::Black;
    g_powercell_leds[0] = (adj_pot[0] < 50) ? CRGB::Red : CRGB::Black;
    g_powercell_leds[NUM_LEDS_POWERCELL - 1] = (adj_pot[1] > 4050) ? CRGB::Green : CRGB::Black;
    g_powercell_leds[NUM_LEDS_POWERCELL - 2] = ((adj_pot[1] > 2450) && (adj_pot[1] <= 4050)) ? CRGB(0, adj_pot[1] >> 4, 0) : CRGB::Black;
    g_powercell_leds[NUM_LEDS_POWERCELL - 3] = ((adj_pot[1] >= 1450) && (adj_pot[1] <= 2450)) ? CRGB(0, adj_pot[1] >> 4, 0) : CRGB::Black;
    g_powercell_leds[NUM_LEDS_POWERCELL - 4] = ((adj_pot[1] < 1450) && (adj_pot[1] >= 50)) ? CRGB(0, adj_pot[1] >> 4, 0) : CRGB::Black;
    g_powercell_leds[NUM_LEDS_POWERCELL - 5] = (adj_pot[1] < 50) ? CRGB::Red : CRGB::Black;
    show_leds();
    sleep_ms(50);
  } while (!fire_sw());

  fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
  show_leds();
  sleep_ms(5);

  sound_start(0x17);
  sound_wait_til_end(false, false);
  sound_start(0x20);
  sound_wait_til_end(false, false);
  powercell_color = CRGB::Blue;
  do {
    g_powercell_leds[0] = (config_dip_sw & 0x01) ? powercell_color + CRGB::Red : CRGB::Black;
    g_powercell_leds[1] = (config_dip_sw & 0x02) ? powercell_color + CRGB::Green : CRGB::Black;
    g_powercell_leds[2] = (config_dip_sw & DIP_HEAT_MASK) ? powercell_color : CRGB::Black;
    g_powercell_leds[3] = (config_dip_sw & DIP_MONSTER_MASK) ? powercell_color + CRGB::Red : CRGB::Black;
    g_powercell_leds[4] = (config_dip_sw & DIP_HUM_MASK) ? powercell_color + CRGB::Green : CRGB::Black;
    g_powercell_leds[10] = pack_pu_sw() ? powercell_color + CRGB::Red : CRGB::Black;
    g_powercell_leds[11] = pu_sw() ? powercell_color + CRGB::Green : CRGB::Black;
    g_powercell_leds[12] = fire_sw() ? powercell_color : CRGB::Black;
    g_powercell_leds[13] = song_sw() ? powercell_color + CRGB::Red : CRGB::Black;
    g_powercell_leds[14] = vent_sw() ? powercell_color + CRGB::Green : CRGB::Black;
    show_leds();
    nsignal_to_wandlights(config_dip_sw != 0);
    sleep_ms(50);
  } while ((config_dip_sw != 0) || !fire_sw());

  fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
  show_leds();
  sleep_ms(5);

  sound_start(0x21);
  sound_wait_til_end(false, false);
  sound_start(0x2A);
  sound_wait_til_end(false, false);
  for (int i = 0; i < 30; i++) {
    vent_light_on(true);
    g_powercell_leds[8] = powercell_color;
    show_leds();
    sleep_ms(50);
    vent_light_on(false);
    g_powercell_leds[8] = powercell_color;
    show_leds();
    sleep_ms(120);
  }
  sound_start(0x2B);
  sound_wait_til_end(false, false);
  sleep_ms(1000);
  sound_start(0x55);
  sound_wait_til_end(false, false);
}
