/**
 * @file pack_state.h
 * @brief Main pack state machine definitions.
 * @details This file defines the core data structures for the pack's state
 *          machine, including the different pack modes (Proton Stream, Slime
 *          Blower, etc.) and the high-level operational states (Off, Idle,
 *          Fire, etc.).
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_STATE_H
#define PACK_STATE_H

#include "animation_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

extern AnimationController g_powercell_controller;
extern AnimationController g_cyclotron_controller;
extern AnimationController g_future_controller;

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of the available firing modes for the pack.
 * @details These typically correspond to different weapon types in TVG mode.
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
 * @brief Enumeration of the high-level operational states of the pack.
 */
typedef enum {
    PS_OFF,             /**< Pack is off. */
    PS_PACK_STANDBY,    /**< Pack is on, wand is off. */
    PS_WAND_STANDBY,    /**< Wand is on, pack is off. */
    PS_IDLE,            /**< Pack and wand are on and idle. */
    PS_FIRE,            /**< Main firing state. */
    PS_FIRE_COOLDOWN,   /**< Afterlife-specific cooldown after firing. */
    PS_SLIME_FIRE,      /**< Firing state for slime-based modes. */
    PS_OVERHEAT,        /**< Overheat state during firing. */
    PS_OVERHEAT_BEEP,   /**< Overheat warning beep state. */
    PS_AUTOVENT,        /**< Automatic venting sequence after overheat. */
    PS_FEEDBACK         /**< Temporary potentiometer feedback animation. */
} PackState;

/**
 * @brief Container for all global pack state information.
 */
typedef struct {
    PackMode startup_mode; /**< The mode the pack should be in at power-up. */
    PackMode mode;         /**< The current active firing mode. */
    PackState state;       /**< The current high-level operational state. */
} PackContext;

/** @brief Global instance of the pack state context. */
extern PackContext pack_ctx;

/**
 * @brief Cyclotron speed multiplier (16.16 fixed-point).
 * @details A value of `1 << 16` represents normal (1.0x) speed. This is used
 *          to briefly slow the cyclotron after firing on Afterlife packs
 *          without affecting other animations.
 */
extern uint32_t cy_speed_multiplier;

/**
 * @brief Schedules a gradual change to the cyclotron speed multiplier.
 * @param target The target speed multiplier (16.16 fixed-point).
 * @param duration The duration of the ramp in milliseconds.
 */
void cy_speed_ramp_go(uint32_t target, unsigned long duration);

/**
 * @brief Updates the cyclotron speed ramp.
 * @details This must be called regularly (e.g., from an ISR) to apply the
 *          gradual speed change scheduled by `cy_speed_ramp_go`.
 */
void cy_speed_ramp_update(void);

/**
 * @brief Initializes the pack state machine to its default state.
 */
void pack_state_init(void);

/**
 * @brief Sets the active firing mode.
 * @param mode The `PackMode` to set as active.
 */
void pack_state_set_mode(PackMode mode);

/**
 * @brief Gets the current firing mode.
 * @return The current `PackMode`.
 */
PackMode pack_state_get_mode(void);

/**
 * @brief Sets the high-level pack state.
 * @param state The `PackState` to set as active.
 */
void pack_state_set_state(PackState state);

/**
 * @brief Gets the current high-level pack state.
 * @return The current `PackState`.
 */
PackState pack_state_get_state(void);

/**
 * @brief Requests the potentiometer feedback animation.
 * @details Invoking this function transitions the pack state to
 *          `PS_FEEDBACK` and schedules the rainbow animation to play on the
 *          cyclotron LEDs.
 */
void feedback_request(void);

/**
 * @brief Runs one iteration of the main state machine.
 * @details This function should be called repeatedly in the main application loop.
 */
void pack_state_process(void);

#ifdef __cplusplus
}
#endif

#endif // PACK_STATE_H
