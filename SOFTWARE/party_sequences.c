/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "party_sequences.h"
#include "addressable_LED_support.h"
#include "colors.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include <stdlib.h>

static party_animation_t current_animation = PARTY_ANIMATION_RAINBOW_FADE;
static bool party_mode_active = false;
static bool party_animation_needs_init = true;

/**
 * @brief Determine the number of active LEDs for a strip.
 *
 * Many animations need to know how many pixels are present on a strip.  Most
 * strips use their maximum length, but the cyclotron can be configured for a
 * smaller ring.  This helper hides that detail by returning the configured
 * pixel count when available and falling back to the hardware maximum
 * otherwise.
 */
static uint8_t active_pixels(LedStrip *strip) {
    return strip->num_pixels ? strip->num_pixels : strip->max_pixels;
}

// --- Animation Implementations ---

static void party_rainbow_fade(bool init) {
    static uint16_t hue = 0;
    if (init) {
        hue = 0;
        return;
    }
    hue = (hue + 1) % 360;
    uint32_t color = hsv_to_grb(hue, 255, 255);

    LedStrip *strips[] = {get_powercell_strip(), get_cyclotron_strip(), get_future_strip()};
    for (int s = 0; s < 3; s++) {
        uint8_t limit = active_pixels(strips[s]);
        for (int i = 0; i < limit; i++) {
            strips[s]->grbx[i] = color;
        }
    }
    start_xfer_all();
}

static void draw_cylon_eye(LedStrip *strip, int center, uint32_t color) {
    uint8_t limit = active_pixels(strip);

    for (int i = 0; i < limit; i++) strip->grbx[i] = 0;
    if (center >= 0 && center < limit) strip->grbx[center] = color;
    if (center > 0) strip->grbx[center - 1] = fade_correction(color, 128);
    if (center < limit - 1) strip->grbx[center + 1] = fade_correction(color, 128);
}

static void party_cylon_scanner(bool init) {
    static int8_t position[3];
    static int8_t direction[3];
    static uint8_t counter = 0;
    static uint32_t eye_color = COLOR_RED;

    if (init) {
        for (int i = 0; i < 3; i++) {
            position[i] = 0;
            direction[i] = 1;
        }
        counter = 0;
        eye_color = hsv_to_grb(rand() % 360, 255, 255);
        return;
    }

    counter = (counter + 1) % 4;
    if (counter != 0) return;

    bool change_color = false;
    LedStrip *strips[] = {get_powercell_strip(), get_cyclotron_strip(), get_future_strip()};
    for (int s = 0; s < 3; s++) {
        uint8_t limit = active_pixels(strips[s]);
        if (limit > 1) {
            position[s] += direction[s];
            if (position[s] >= limit - 1) {
                direction[s] = -1;
            } else if (position[s] <= 0) {
                direction[s] = 1;
                change_color = true;
            }
        } else {
            position[s] = 0;
        }
    }

    if (change_color) {
        eye_color = hsv_to_grb(rand() % 360, 255, 255);
    }

    for (int s = 0; s < 3; s++) {
        draw_cylon_eye(strips[s], position[s], eye_color);
    }

    start_xfer_all();
}

static void party_random_sparkle(bool init) {
    LedStrip *strips[] = {get_powercell_strip(), get_cyclotron_strip(), get_future_strip()};
    for (int s = 0; s < 3; s++) {
        uint8_t limit = active_pixels(strips[s]);
        for (int i = 0; i < limit; i++) {
            strips[s]->grbx[i] = fade_correction(strips[s]->grbx[i], 32);
        }
    }

    if ((rand() % 100) < 30) {
        LedStrip *strip = strips[rand() % 3];
        uint8_t limit = active_pixels(strip);
        if (limit > 0) {
            int i = rand() % limit;
            strip->grbx[i] = hsv_to_grb(rand() % 360, 255, 255);
        }
    }
    start_xfer_all();
}

static void party_beat_meter(bool init) {
    static int16_t level = 0;
    static int8_t direction = 1;
    static uint32_t color = 0;
    static uint8_t counter = 0;

    LedStrip *strips[] = {get_powercell_strip(), get_cyclotron_strip(), get_future_strip()};
    uint8_t max_level = active_pixels(strips[0]);
    for (int s = 1; s < 3; s++) {
        uint8_t count = active_pixels(strips[s]);
        if (count > max_level) max_level = count;
    }

    if (init) {
        level = 0;
        direction = 1;
        color = hsv_to_grb(rand() % 360, 255, 255);
        counter = 0;
        return;
    }

    counter = (counter + 1) % 2;
    if (counter != 0) return;

    level += direction;
    if (direction > 0 && level >= max_level) {
        level = max_level - 1;
        direction = -1;
    } else if (direction < 0 && level <= 0) {
        level = 0;
        direction = 1;
        color = hsv_to_grb(rand() % 360, 255, 255);
    }

    for (int s = 0; s < 3; s++) {
        clear_strip(strips[s]);
        uint8_t limit = active_pixels(strips[s]);
        uint8_t threshold = ((level + 1) * limit) / max_level;
        for (int i = 0; i < threshold; i++) {
            strips[s]->grbx[i] = color;
        }
    }
    start_xfer_all();
}


// --- Main Party Mode Control ---

void party_mode_run(void) {
    if (!party_mode_active) return;

    if (party_animation_needs_init) {
        switch (current_animation) {
            case PARTY_ANIMATION_RAINBOW_FADE:
                party_rainbow_fade(true);
                break;
            case PARTY_ANIMATION_CYLON_SCANNER:
                party_cylon_scanner(true);
                break;
            case PARTY_ANIMATION_RANDOM_SPARKLE:
                party_random_sparkle(true);
                break;
            case PARTY_ANIMATION_BEAT_METER:
                party_beat_meter(true);
                break;
            case PARTY_ANIMATION_COUNT:
                break;
        }
        party_animation_needs_init = false;
    }

    switch (current_animation) {
        case PARTY_ANIMATION_RAINBOW_FADE:
            party_rainbow_fade(false);
            break;
        case PARTY_ANIMATION_CYLON_SCANNER:
            party_cylon_scanner(false);
            break;
        case PARTY_ANIMATION_RANDOM_SPARKLE:
            party_random_sparkle(false);
            break;
        case PARTY_ANIMATION_BEAT_METER:
            party_beat_meter(false);
            break;
        case PARTY_ANIMATION_COUNT:
            break;
    }
}

void party_mode_set_animation(party_animation_t animation) {
    if (animation >= PARTY_ANIMATION_COUNT) {
        party_mode_stop();
        return;
    }

    party_mode_active = false;
    pc_pattern_stop(true);
    cy_pattern_stop(true);
    fr_pattern_stop(true);
    clear_led_vars();

    current_animation = animation;
    party_mode_active = true;
    party_animation_needs_init = true;
}

void party_mode_stop(void) {
    if (!party_mode_active) {
        return;
    }

    party_mode_active = false;
    pc_pattern_stop(true);
    cy_pattern_stop(true);
    fr_pattern_stop(true);
    clear_led_vars();
}

bool party_mode_is_active(void) {
    return party_mode_active;
}
