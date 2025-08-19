/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

// Helper to pack RGB components for the hardware's GRB order. The returned
// 32-bit value is left-aligned so it can be written directly to the PIO FIFO
// which expects the colour in the upper 24 bits.
#define PACK_RGB_TO_GRB32(r, g, b)                                             \
    ((((uint32_t)(g)) << 24) | (((uint32_t)(r)) << 16) |                      \
     (((uint32_t)(b)) << 8))

// Defines our standard colour palette using 32-bit GRB values
typedef enum {
    COLOR_RED    = PACK_RGB_TO_GRB32(255, 0,   0),
    COLOR_GREEN  = PACK_RGB_TO_GRB32(0,   255, 0),
    COLOR_BLUE   = PACK_RGB_TO_GRB32(0,   0,   255),
    COLOR_WHITE  = PACK_RGB_TO_GRB32(255, 255, 255),
    COLOR_ORANGE = PACK_RGB_TO_GRB32(255, 128, 0),
} Color;

#endif // COLORS_H
