/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: addressable_LED_support.c
 * Version 1.0.0
 * Date: 2025-05-01
 *
 * This file is part of the GhostLab42 Support for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 *
 * The Sound module's addressable LEDs are powered by a 5V regulator that is
 * powered by a 12V battery.  If only the USB is connected and powering the
 * Raspberry Pi Pico, the addressable LEDs will not be powered and will not
 * light up.
 */

#include "addressable_LED_support.h"
#include "generated/ws2812.pio.h"
#include "pico/stdlib.h"
#include "colors.h"

const CyclotronPositionMap cyclotron_positions = {
    .classic = {
        {1, 2, 3, 4, 4},     // 4 lights only
        {4, 10, 14, 20, 24}, // 24 lights only
        {5, 13, 19, 27, 32}, // 32 lights only
        {6, 16, 24, 34, 40}  // 40 lights only
    }};

// The LedPalette struct has been removed and is replaced by the Color enum in colors.h

// This is the gamma correction table.
static const uint8_t gamma_table[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,  12,  13,  14,  15,
    16, 17, 18, 19, 20, 20, 21, 21, 22, 22, 23, 23,  24,  24,  25,  25,
    26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 32,  32,  33,  33,  34,
    35, 35, 36, 36, 37, 38, 38, 39, 40, 40, 41, 42,  42,  43,  44,  45,
    45, 46, 47, 48, 48, 49, 50, 51, 52, 52, 53, 54,  55,  56,  57,  58,
    59, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,  70,  71,  72,  73,
    74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86,  87,  88,  89,  90,
    91, 93, 94, 95, 96, 97, 98, 100,101,102,103,105, 106, 107, 108, 110,
    111,112,113,115,116,117,118,120,121,122,124,125, 126, 128, 129, 130,
    132,133,134,136,137,138,140,141,143,144,145,147, 148, 150, 151, 153,
    154,155,157,158,160,161,163,164,166,167,169,170, 172, 173, 175, 176,
    178,179,181,182,184,185,187,188,190,192,193,195, 196, 198, 199, 201,
    202,204,206,207,209,210,212,214,215,217,219,220, 222, 224, 225, 227,
    229,230,232,234,235,237,239,241,242,244,246,248, 249, 251, 253, 255
};


// LED array variables, 4 bytes per entry.
static uint32_t powercell_buffer[MAX_PIXELS_POWERCELL];
static uint32_t cyclotron_buffer[MAX_PIXELS_CYCLOTRON];
static uint32_t future_buffer[MAX_PIXELS_FUTURE];

static LedStrip powercell_strip = {
    .num_pixels = MAX_PIXELS_POWERCELL,
    .max_pixels = MAX_PIXELS_POWERCELL,
    .num_bits = NUM_BITS_POWERCELL,
    .grbx = powercell_buffer,
    .pio = NULL,
    .sm = 0,
    .offset = 0,
    .dma_channel = 0,
};

static LedStrip cyclotron_strip = {
    .num_pixels = MAX_PIXELS_CYCLOTRON,
    .max_pixels = MAX_PIXELS_CYCLOTRON,
    .num_bits = NUM_BITS_CYCLOTRON,
    .grbx = cyclotron_buffer,
    .pio = NULL,
    .sm = 0,
    .offset = 0,
    .dma_channel = 0,
};

static LedStrip future_strip = {
    .num_pixels = MAX_PIXELS_FUTURE,
    .max_pixels = MAX_PIXELS_FUTURE,
    .num_bits = NUM_BITS_FUTURE,
    .grbx = future_buffer,
    .pio = NULL,
    .sm = 0,
    .offset = 0,
    .dma_channel = 0,
};

// Accessors provide controlled visibility of LED strip instances
LedStrip *get_powercell_strip(void) { return &powercell_strip; }

LedStrip *get_cyclotron_strip(void) { return &cyclotron_strip; }

LedStrip *get_future_strip(void) { return &future_strip; }

// Just for testing, be able to manually feed data to the PIO
// This is not used in the final code, but can be used to test the PIO program
static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
  pio_sm_put_blocking(pio, sm, pixel_grb);
}

// Clear an LED strip buffer
void clear_strip(volatile LedStrip *strip) {
  for (int i = 0; i < strip->max_pixels; i++) {
    strip->grbx[i] = 0;
  }
}

// This is used to clear all the LED arrays to 0
void clear_led_vars(void) {
  clear_strip(&powercell_strip); // clear the powercell vars
  clear_strip(&cyclotron_strip); // clear the cyclotron vars
  clear_strip(&future_strip);    // clear the future vars
}

uint32_t hsv_to_grb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        r = g = b = v;
    } else {
        region = h / 60;
        remainder = (h % 60) * 255 / 60;

        p = (v * (255 - s)) >> 8;
        q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
    // Pack into GRB format left-aligned for the PIO
    return PACK_RGB_TO_GRB32(r, g, b);
}


/** @copydoc fade_correction */
uint32_t fade_correction(uint32_t color, uint8_t fade_amount) {
    if (fade_amount == 0) return color;
    if (fade_amount == 255) return 0;

    uint8_t g = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t b = (color >> 8) & 0xFF;

    uint8_t fade_index = gamma_table[255 - fade_amount];

    r = (r * fade_index) >> 8;
    g = (g * fade_index) >> 8;
    b = (b * fade_index) >> 8;

    return PACK_RGB_TO_GRB32(r, g, b);
}

// Initialize a strip's PIO state machine and DMA channel
bool init_strip(LedStrip *strip, const pio_program_t *program, uint gpio_pin) {
  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
      program, &strip->pio, &strip->sm, &strip->offset, gpio_pin, 1, true);
  if (!success) {
    return false;
  }

  strip->dma_channel = dma_claim_unused_channel(false);
  if ((int)strip->dma_channel < 0) {
    return false;
  }
  strip->dma_config = dma_channel_get_default_config(strip->dma_channel);
  channel_config_set_transfer_data_size(&strip->dma_config, DMA_SIZE_32);
  channel_config_set_read_increment(&strip->dma_config, true);
  channel_config_set_write_increment(&strip->dma_config, false);
  channel_config_set_dreq(&strip->dma_config,
                          pio_get_dreq(strip->pio, strip->sm, true));
  dma_channel_configure(strip->dma_channel, &strip->dma_config,
                        &strip->pio->txf[strip->sm], strip->grbx,
                        strip->max_pixels, false);

  return true;
}

/** @copydoc init_leds */
void init_leds(void) {
  // Clear all LED variables
  clear_led_vars();

  bool success = init_strip((LedStrip *)&powercell_strip, &powercell_program,
                            powercell_GPIO_N);
  hard_assert(success);
  powercell_program_init(powercell_strip.pio, powercell_strip.sm,
                         powercell_strip.offset, powercell_strip.num_bits);

  success = init_strip((LedStrip *)&cyclotron_strip, &cyclotron_program,
                       cyclotron_GPIO_N);
  hard_assert(success);
  cyclotron_program_init(cyclotron_strip.pio, cyclotron_strip.sm,
                         cyclotron_strip.offset, cyclotron_strip.num_bits);

  success =
      init_strip((LedStrip *)&future_strip, &future_program, future_GPIO_N);
  hard_assert(success);
  future_program_init(future_strip.pio, future_strip.sm, future_strip.offset,
                      future_strip.num_bits);

  // some strings did not get cleared from this initial startup sequence
  blank_all_strings();
  sleep_ms(5);
}

/** @copydoc start_xfer_powercell */
void start_xfer_powercell(void) {
  if (dma_channel_is_busy(powercell_strip.dma_channel))
    dma_channel_abort(powercell_strip.dma_channel);
  dma_channel_transfer_from_buffer_now(powercell_strip.dma_channel,
                                       powercell_strip.grbx,
                                       powercell_strip.max_pixels);
}
/** @copydoc start_xfer_cyclotron */
void start_xfer_cyclotron(void) {
  if (dma_channel_is_busy(cyclotron_strip.dma_channel))
    dma_channel_abort(cyclotron_strip.dma_channel);
  dma_channel_transfer_from_buffer_now(cyclotron_strip.dma_channel,
                                       cyclotron_strip.grbx,
                                       cyclotron_strip.max_pixels);
}
/** @copydoc start_xfer_future */
void start_xfer_future(void) {
  if (dma_channel_is_busy(future_strip.dma_channel))
    dma_channel_abort(future_strip.dma_channel);
  dma_channel_transfer_from_buffer_now(
      future_strip.dma_channel, future_strip.grbx, future_strip.max_pixels);
}
/** @copydoc start_xfer_all */
void start_xfer_all(void) {
  dma_channel_wait_for_finish_blocking(powercell_strip.dma_channel);
  dma_channel_wait_for_finish_blocking(cyclotron_strip.dma_channel);
  dma_channel_wait_for_finish_blocking(future_strip.dma_channel);
  start_xfer_cyclotron(); // start a transfer for cyclotron
  start_xfer_powercell(); // start a transfer for powercell
  start_xfer_future();    // start a transfer for future
}

/** @copydoc blank_powercell_string */
void blank_powercell_string(void) {
  // This will blank the powercell string by sending a full transfer of 0s to
  // the PIO
  clear_strip(&powercell_strip); // clear the powercell vars
  start_xfer_powercell();        // start a transfer for powercell
}

/** @copydoc blank_cyclotron_string */
void blank_cyclotron_string(void) {
  // This will blank the cyclotron string by sending a full transfer of 0s to
  // the PIO
  clear_strip(&cyclotron_strip); // clear the cyclotron vars
  start_xfer_cyclotron();        // start a transfer for cyclotron
}

/** @copydoc blank_future_string */
void blank_future_string(void) {
  // This will blank the future string by sending a full transfer of 0s to the
  // PIO
  clear_strip(&future_strip); // clear the future vars
  start_xfer_future();        // start a transfer for future
}

/** @copydoc blank_all_strings */
void blank_all_strings(void) {
  // This will blank all the strings by sending a full transfer of 0s to the
  // PIOs
  clear_led_vars(); // clear all three LED arrays
  start_xfer_all(); // start a transfer for all three LED connections
  dma_channel_wait_for_finish_blocking(powercell_strip.dma_channel);
  dma_channel_wait_for_finish_blocking(cyclotron_strip.dma_channel);
  dma_channel_wait_for_finish_blocking(future_strip.dma_channel);
}
