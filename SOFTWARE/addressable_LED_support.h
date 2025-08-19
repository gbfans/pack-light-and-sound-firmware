/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef ADDRESSABLE_LED_SUPPORT_H
#define ADDRESSABLE_LED_SUPPORT_H

#include "hardware/dma.h"
#include "hardware/pio.h"
#include <stdint.h>

enum {
  MAX_PIXELS_POWERCELL = 15,
  NUM_BITS_POWERCELL = 24,
  MAX_PIXELS_CYCLOTRON = 40,
  NUM_BITS_CYCLOTRON = 24,
  MAX_PIXELS_FUTURE = 16,
  NUM_BITS_FUTURE = 24,
};

/* Backward compatibility for legacy constants. */
#define NUM_PIXELS_POWERCELL MAX_PIXELS_POWERCELL
#define MAX_NUM_PIXELS_CYCLOTRON MAX_PIXELS_CYCLOTRON
#define NUM_PIXELS_FUTURE MAX_PIXELS_FUTURE

typedef struct {
  uint8_t classic[4][5];
} CyclotronPositionMap;

extern const CyclotronPositionMap cyclotron_positions;

typedef struct {
  uint8_t num_pixels;     /**< Soft limit: active pixels used by patterns. */
  uint8_t max_pixels;     /**< Hardware maximum for the strip. */
  uint8_t num_bits;
  uint32_t *grbx;
  PIO pio;
  uint sm;
  uint offset;
  uint dma_channel;
  dma_channel_config dma_config;
} LedStrip;

LedStrip *get_powercell_strip(void);
LedStrip *get_cyclotron_strip(void);
LedStrip *get_future_strip(void);

/**
 * @brief Clear an LED strip buffer.
 *
 * Sets all entries in the provided strip buffer to zero.
 *
 * @param strip LED strip to clear.
 */
void clear_strip(volatile LedStrip *strip);

/**
 * @brief Clear all LED buffers.
 *
 * Clears the buffers for powercell, cyclotron and future strips.
 */
void clear_led_vars(void);

/**
 * @brief Convert HSV colour to GRB format.
 *
 * The returned value is left-aligned for direct transmission to the PIO
 * peripheral.
 *
 * @param h Hue (0-359)
 * @param s Saturation (0-255)
 * @param v Value (0-255)
 * @return 32-bit GRB colour value.
 */
uint32_t hsv_to_grb(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief Apply gamma-corrected fading to a colour value.
 *
 * @param color GRB colour value to adjust.
 * @param fade_amount Fade level; 0 = no fade, 255 = fully off.
 * @return Faded GRB colour.
 */
uint32_t fade_correction(uint32_t color, uint8_t fade_amount);

/**
 * @brief Initialize a strip's PIO state machine and DMA channel.
 *
 * Configures a PIO and DMA channel for the specified strip.
 *
 * @param strip     LED strip descriptor to initialize.
 * @param program   PIO program controlling the strip.
 * @param gpio_pin  GPIO pin used for the strip.
 * @return true on success, false otherwise.
 */
bool init_strip(LedStrip *strip, const pio_program_t *program, uint gpio_pin);

/**
 * @brief Initialize LED hardware and buffers.
 *
 * Configures PIO programs and DMA channels and clears all LED buffers.
 */
void init_leds(void);

/**
 * @brief Start DMA transfer for the powercell LED strip.
 *
 */
void start_xfer_powercell(void);

/**
 * @brief Start DMA transfer for the cyclotron LED strip.
 *
 */
void start_xfer_cyclotron(void);

/**
 * @brief Start DMA transfer for the future LED strip.
 *
 */
void start_xfer_future(void);

/**
 * @brief Start DMA transfers for all LED strips.
 *
 */
void start_xfer_all(void);

/**
 * @brief Clear and blank the powercell LED strip.
 *
 * Clears the buffer and sends a blank frame to the powercell LEDs.
 *
 */
void blank_powercell_string(void);

/**
 * @brief Clear and blank the cyclotron LED strip.
 *
 * Clears the buffer and sends a blank frame to the cyclotron LEDs.
 *
 */
void blank_cyclotron_string(void);

/**
 * @brief Clear and blank the future LED strip.
 *
 * Clears the buffer and sends a blank frame to the future LEDs.
 *
 */
void blank_future_string(void);

/**
 * @brief Clear and blank all LED strips.
 *
 * Clears all buffers and sends blank frames to every LED strip.
 *
 */
void blank_all_strings(void);

#endif // ADDRESSABLE_LED_SUPPORT_H
