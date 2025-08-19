/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PARTY_SEQUENCES_H
#define PARTY_SEQUENCES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PARTY_ANIMATION_RAINBOW_FADE,
    PARTY_ANIMATION_CYLON_SCANNER,
    PARTY_ANIMATION_RANDOM_SPARKLE,
    PARTY_ANIMATION_BEAT_METER,
    PARTY_ANIMATION_COUNT
} party_animation_t;

void party_mode_run(void);
void party_mode_set_animation(party_animation_t animation);
void party_mode_stop(void);
bool party_mode_is_active(void);

#endif // PARTY_SEQUENCES_H
