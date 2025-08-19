/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_STATE_H
#define PACK_STATE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Available firing modes for the pack.
 */
typedef enum {
    PACK_MODE_PROTON_STREAM = 0,
    PACK_MODE_BOSON_DART,
    PACK_MODE_SLIME_BLOWER,
    PACK_MODE_SLIME_TETHER,
    PACK_MODE_STASIS_STREAM,
    PACK_MODE_SHOCK_BLAST,
    PACK_MODE_OVERLOAD_PULSE,
    PACK_MODE_MESON_COLLIDER
} PackMode;

/**
 * @brief High level operational states.
 */
typedef enum {
    PS_OFF,
    PS_PACK_STANDBY,
    PS_WAND_STANDBY,
    PS_IDLE,
    PS_FIRE,
    PS_FIRE_COOLDOWN,
    PS_SLIME_FIRE,
    PS_OVERHEAT,
    PS_OVERHEAT_BEEP,
    PS_AUTOVENT
} PackState;

/**
 * @brief Container for all pack state information.
 */
typedef struct {
    PackMode startup_mode; /**< Mode at power-up. */
    PackMode mode;         /**< Current firing mode. */
    PackState state;       /**< Current high-level state. */
} PackContext;

extern PackContext pack_ctx; /**< Global pack state context. */

/**
 * @brief Cyclotron speed multiplier (16.16 fixed-point).
 *
 * A value of 1 << 16 represents normal speed. Used to briefly slow the
 * cyclotron after firing on Afterlife packs without affecting other
 * animations.
 */
extern uint32_t cy_speed_multiplier;

/** Initialize the pack state machine. */
void pack_state_init(void);

/** Set the active firing mode. */
void pack_state_set_mode(PackMode mode);

/** Get the current firing mode. */
PackMode pack_state_get_mode(void);

/** Set the high-level pack state. */
void pack_state_set_state(PackState state);

/** Get the high-level pack state. */
PackState pack_state_get_state(void);

/** Run one iteration of the state machine. */
void pack_state_process(void);

#endif // PACK_STATE_H
