/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "board_test.h"
#include "addressable_LED_support.h"
#include "klystron_IO_support.h"
#include "pico/stdlib.h"
#include "sound_module.h"
#include "hardware/dma.h"

#define powercell_strip (*get_powercell_strip())
#define cyclotron_strip (*get_cyclotron_strip())
#define future_strip (*get_future_strip())

/**
 * @brief Display a solid color across all LED strips.
 *
 * @param color GRBX color value to show.
 * @param delay Delay in milliseconds between pixel updates.
 */
static void test_color(int32_t color, int delay) {
  for (int i = 0; i < NUM_PIXELS_POWERCELL; i++) {
    powercell_strip.grbx[i] = color;
    start_xfer_powercell();
    dma_channel_wait_for_finish_blocking(powercell_strip.dma_channel);
    sleep_ms(delay);
  }
  for (int i = 0; i < cyclotron_strip.num_pixels; i++) {
    cyclotron_strip.grbx[i] = color;
    start_xfer_cyclotron();
    dma_channel_wait_for_finish_blocking(cyclotron_strip.dma_channel);
    sleep_ms(delay);
  }
  for (int i = 0; i < NUM_PIXELS_FUTURE; i++) {
    future_strip.grbx[i] = color;
    start_xfer_future();
    dma_channel_wait_for_finish_blocking(future_strip.dma_channel);
    sleep_ms(delay);
  }
}

/**
 * @brief Run the hardware verification routine.
 */
void board_test(void) {
  uint32_t powercell_color = 0x3F << 8;
  sound_start(0x0);
  sound_wait_til_end(false, false);
  powercell_strip.grbx[NUM_PIXELS_POWERCELL - 1] = powercell_color | 0x3F << 24;
  powercell_strip.grbx[0] = powercell_color | 0x3F << 16;
  start_xfer_powercell();
  sleep_ms(1000);
  sound_start(0x1);
  sound_wait_til_end(false, false);
  sound_start(0x0C);
  sound_wait_til_end(false, false);
  sound_start(0x1);
  sound_wait_til_end(false, false);
  sound_start(0x0);
  sound_wait_til_end(false, false);
  sleep_ms(2000);

  const int delay = 25;
  clear_led_vars();
  test_color(0x7F << 16, delay);
  test_color(0x7F << 24, delay);
  test_color(0x7F << 8, delay);
  sleep_ms(1000);

  clear_led_vars();
  powercell_strip.grbx[0] = 0x7F << 16;
  powercell_strip.grbx[1] = 0x7F << 24;
  powercell_strip.grbx[2] = 0x7F << 8;
  powercell_strip.grbx[NUM_PIXELS_POWERCELL - 2] = 0x7F << 16 | 0x7F << 8;
  powercell_strip.grbx[NUM_PIXELS_POWERCELL - 1] = 0x7F << 16 | 0x7F << 24;
  cyclotron_strip.grbx[0] = 0x7F << 16;
  cyclotron_strip.grbx[1] = 0x7F << 24;
  cyclotron_strip.grbx[2] = 0x7F << 8;
  cyclotron_strip.grbx[cyclotron_strip.num_pixels - 2] = 0x7F << 16 | 0x7F << 8;
  cyclotron_strip.grbx[cyclotron_strip.num_pixels - 1] = 0x7F << 16 | 0x7F
                                                                          << 24;
  future_strip.grbx[0] = 0x7F << 16;
  future_strip.grbx[1] = 0x7F << 24;
  future_strip.grbx[2] = 0x7F << 8;
  future_strip.grbx[NUM_PIXELS_FUTURE - 2] = 0x7F << 16 | 0x7F << 8;
  future_strip.grbx[NUM_PIXELS_FUTURE - 1] = 0x7F << 16 | 0x7F << 24;
  start_xfer_all();
  sleep_ms(5000);
  clear_led_vars();
  start_xfer_all();
  sleep_ms(5);

  sound_start(0x17);
  sound_wait_til_end(false, false);
  do {
    read_adj_potentiometers(true);
    powercell_strip.grbx[4] = (adj_pot[0] > 4050) ? 0x7F << 24 : 0;
    powercell_strip.grbx[3] =
        ((adj_pot[0] > 2450) && (adj_pot[0] <= 4050)) ? adj_pot[0] << 3 : 0;
    powercell_strip.grbx[2] =
        ((adj_pot[0] >= 1450) && (adj_pot[0] <= 2450)) ? adj_pot[0] << 3 : 0;
    powercell_strip.grbx[1] =
        ((adj_pot[0] < 1450) && (adj_pot[0] >= 50)) ? adj_pot[0] << 3 : 0;
    powercell_strip.grbx[0] = (adj_pot[0] < 50) ? 0x7F << 16 : 0;
    powercell_strip.grbx[NUM_PIXELS_POWERCELL - 1] =
        (adj_pot[1] > 4050) ? 0x7F << 24 : 0;
    powercell_strip.grbx[NUM_PIXELS_POWERCELL - 2] =
        ((adj_pot[1] > 2450) && (adj_pot[1] <= 4050)) ? adj_pot[1] << 3 : 0;
    powercell_strip.grbx[NUM_PIXELS_POWERCELL - 3] =
        ((adj_pot[1] >= 1450) && (adj_pot[1] <= 2450)) ? adj_pot[1] << 3 : 0;
    powercell_strip.grbx[NUM_PIXELS_POWERCELL - 4] =
        ((adj_pot[1] < 1450) && (adj_pot[1] >= 50)) ? adj_pot[1] << 3 : 0;
    powercell_strip.grbx[NUM_PIXELS_POWERCELL - 5] =
        (adj_pot[1] < 50) ? 0x7F << 16 : 0;
    start_xfer_powercell();
    sleep_ms(50);
  } while (!fire_sw());
  clear_led_vars();
  start_xfer_all();
  sleep_ms(5);

  sound_start(0x17);
  sound_wait_til_end(false, false);
  sound_start(0x20);
  sound_wait_til_end(false, false);
  powercell_color = 0x3F << 8;
  do {
    powercell_strip.grbx[0] =
        (config_dip_sw & 0x01) ? powercell_color | 0x3F << 16 : 0;
    powercell_strip.grbx[1] =
        (config_dip_sw & 0x02) ? powercell_color | 0x3F << 24 : 0;
    powercell_strip.grbx[2] =
        (config_dip_sw & DIP_HEAT_MASK) ? powercell_color : 0;
    powercell_strip.grbx[3] =
        (config_dip_sw & DIP_MONSTER_MASK) ? powercell_color | 0x3F << 16 : 0;
    powercell_strip.grbx[4] =
        (config_dip_sw & DIP_HUM_MASK) ? powercell_color | 0x3F << 24 : 0;
    powercell_strip.grbx[10] = pack_pu_sw() ? powercell_color | 0x3F << 16 : 0;
    powercell_strip.grbx[11] = pu_sw() ? powercell_color | 0x3F << 24 : 0;
    powercell_strip.grbx[12] = fire_sw() ? powercell_color : 0;
    powercell_strip.grbx[13] = song_sw() ? powercell_color | 0x3F << 16 : 0;
    powercell_strip.grbx[14] = vent_sw() ? powercell_color | 0x3F << 24 : 0;
    start_xfer_all();
    nsignal_to_wandlights(config_dip_sw != 0);
    sleep_ms(50);
  } while ((config_dip_sw != 0) || !fire_sw());
  clear_led_vars();
  start_xfer_all();
  sleep_ms(5);

  sound_start(0x21);
  sound_wait_til_end(false, false);
  sound_start(0x2A);
  sound_wait_til_end(false, false);
  for (int i = 0; i < 30; i++) {
    vent_light_on(true);
    powercell_strip.grbx[8] = powercell_color;
    start_xfer_powercell();
    sleep_ms(50);
    vent_light_on(false);
    powercell_strip.grbx[8] = 0;
    start_xfer_powercell();
    sleep_ms(120);
  }
  sound_start(0x2B);
  sound_wait_til_end(false, false);
  sleep_ms(1000);
  sound_start(0x55);
  sound_wait_til_end(false, false);
}
