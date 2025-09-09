/**
 * @file klystron_IO_support.h
 * @brief Hardware I/O definitions and functions.
 * @details This file defines GPIO pin assignments, DIP switch masks, and provides
 *          the interface for all low-level hardware I/O operations, including
 *          reading potentiometers, debouncing switches, and controlling simple
 *          GPIO outputs.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef KLYSTRON_IO_SUPPORT_H
#define KLYSTRON_IO_SUPPORT_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// === GPIO assignments ===
static const uint GPO_NBUSY_TO_WAND = 12;
static const uint GPO_VENT_LIGHT = 28;
static const uint GPO_MUTE = 22;

// === DIP switch masks ===
static const uint8_t DIP_PACKSEL0_MASK = 0x01;
static const uint8_t DIP_PACKSEL1_MASK = 0x02;
static const uint8_t DIP_PACKSEL_MASK =
    (DIP_PACKSEL0_MASK | DIP_PACKSEL1_MASK);
static const uint8_t DIP_HEAT_MASK = 0x04;
static const uint8_t DIP_MONSTER_MASK = 0x08;
static const uint8_t DIP_HUM_MASK = 0x10;

/** @brief Enumeration of the different pack types selectable by DIP switch. */
typedef enum {
    PACK_TYPE_SNAP_RED = 0,
    PACK_TYPE_FADE_RED,
    PACK_TYPE_TVG_FADE,
    PACK_TYPE_AFTERLIFE,
    PACK_TYPE_AFTER_TVG
} PackType;

// === Global I/O state variables ===
extern volatile uint16_t adj_pot[2];
extern volatile uint8_t config_dip_sw;
extern volatile uint8_t user_switches;
extern volatile uint8_t user_switch_flags;

// === Function Prototypes ===

void read_adj_potentiometers(bool average);
void init_adc(void);
void init_gpio(void);
void check_dip_switches_isr(void);
void check_user_switches_isr(void);

// --- Switch state accessors ---
bool pack_pu_sw(void);
bool pack_pu_req(void);
bool pu_sw(void);
bool fire_sw(void);
bool fire_tap(void);
bool song_sw(void);
bool song_toggle(void);
bool vent_sw(void);
bool wand_standby_sw(void);

// --- Flag clearing functions ---
void clear_fire_tap(void);
void clear_song_toggle(void);
void clear_pack_pu_req(void);

// --- Direct GPIO control ---
void nsignal_to_wandlights(bool autovent);
void vent_light_on(bool turn_on);
void mute_audio(void);
void unmute_audio(void);

// --- Configuration accessors ---
PackType config_pack_type(void);
uint8_t config_cyclotron_dir(void);

#ifdef __cplusplus
}
#endif

#endif // KLYSTRON_IO_SUPPORT_H
