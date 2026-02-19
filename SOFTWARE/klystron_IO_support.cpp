/**
 * @file klystron_IO_support.cpp
 * @brief Implements hardware I/O functions.
 * @details This file implements the functions for all low-level hardware
 *          I/O operations, including reading potentiometers, debouncing
 *          switches, and controlling simple GPIO outputs.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "klystron_IO_support.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// === Global I/O state variables ===
/** @brief Smoothed ADC readings for the two potentiometers. */
volatile uint16_t adj_pot[2] = {0, 0};
/** @brief Debounced state of the 5-position DIP switch block. */
volatile uint8_t config_dip_sw = 0;
/** @brief Debounced state of the user-facing switches (power, fire, etc.). */
volatile uint8_t user_switches = 0;
/** @brief Flags for single-press events (toggles, taps). */
volatile uint8_t user_switch_flags = 0;

/**
 * @brief Reads the two ADC channels connected to the potentiometers.
 * @details It maintains a moving average of the last 4 readings to provide
 *          a smoothed, more stable output value.
 * @param average If true, returns the smoothed average. If false, returns
 *                the raw instantaneous reading.
 */
void read_adj_potentiometers(bool average) {
    static uint16_t multiple_readings[2][4] = {0};
    for (int i = 3; i >= 1; i--) {
        multiple_readings[0][i] = multiple_readings[0][i - 1];
        multiple_readings[1][i] = multiple_readings[1][i - 1];
    }
    adc_select_input(0);
    multiple_readings[0][0] = adc_read();
    adc_select_input(1);
    multiple_readings[1][0] = adc_read();
    adj_pot[0] =
        average ? (multiple_readings[0][0] + multiple_readings[0][1] +
                   multiple_readings[0][2] + multiple_readings[0][3] + 2) >> 2
                : multiple_readings[0][0];
    adj_pot[1] =
        average ? (multiple_readings[1][0] + multiple_readings[1][1] +
                   multiple_readings[1][2] + multiple_readings[1][3] + 2) >> 2
                : multiple_readings[1][0];
}

/**
 * @brief Initializes the ADC hardware.
 * @details Configures the ADC and the two GPIO pins (26, 27) used for
 *          potentiometer inputs. It performs several initial readings to
 *          populate the smoothing buffer.
 */
void init_adc(void) {
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
}

/**
 * @brief Initializes all GPIO pins for their intended functions.
 * @details Configures switch inputs with pull-ups and sets output pins
 *          to their default states.
 */
void init_gpio(void) {
    for (int gpio = 6; gpio <= 10; gpio++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
    gpio_init(11);
    gpio_set_dir(11, GPIO_IN);
    gpio_pull_up(11);
    for (int gpio = 13; gpio <= 16; gpio++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
    gpio_init(GPO_NBUSY_TO_WAND);
    gpio_set_dir(GPO_NBUSY_TO_WAND, GPIO_OUT);
    gpio_put(GPO_NBUSY_TO_WAND, 1);
    gpio_init(GPO_VENT_LIGHT);
    gpio_set_dir(GPO_VENT_LIGHT, GPIO_OUT);
    gpio_put(GPO_VENT_LIGHT, 0);
    gpio_init(GPO_MUTE);
    gpio_put(GPO_MUTE, 1);
    gpio_set_dir(GPO_MUTE, GPIO_OUT);
}

/**
 * @brief ISR-based function to read and debounce the DIP switches.
 * @details Called by the repeating timer ISR to poll the DIP switch GPIOs
 *          and update the global `config_dip_sw` variable after a debounce
 *          period.
 */
void check_dip_switches_isr(void) {
    static uint8_t config_dip_last = 0;
    static uint8_t config_dip_maybe = 0;
    static uint8_t debounce_dip_cnt = 0;
    const uint8_t debounce_dip_done = 10;
    config_dip_maybe = 0;
    for (int gpio = 6; gpio <= 10; gpio++) {
        config_dip_maybe |= (gpio_get(gpio) << (10 - gpio));
    }
    config_dip_maybe = 0x1F & (~config_dip_maybe);
    if (config_dip_maybe != config_dip_sw) {
        if (config_dip_maybe != config_dip_last) {
            debounce_dip_cnt = 0;
            config_dip_last = config_dip_maybe;
        }
        debounce_dip_cnt++;
        if (debounce_dip_cnt >= debounce_dip_done) {
            config_dip_sw = config_dip_maybe;
            debounce_dip_cnt = 0;
        }
    } else {
        debounce_dip_cnt = 0;
    }
}

/**
 * @brief ISR-based function to read and debounce the user-facing switches.
 * @details Called by the repeating timer ISR. It debounces the main switches
 *          and also implements special logic for detecting "taps" (short
 *          press-and-release events) on the fire switch and "toggles" (state
 *          changes) on the song switch.
 */
void check_user_switches_isr(void) {
    static uint8_t config_user_last = 0;
    static uint8_t debounce_user_cnt = 0;
    static uint8_t debounce_fire_cnt = 0;
    static bool user_inputs_initialized = false;
    const uint8_t debounce_user_done = 15;
    const uint8_t debounce_fire_found = 12;
    const uint8_t debounce_fire_max = 30;

    uint8_t config_user_maybe = gpio_get(11);
    for (int gpio = 13; gpio <= 16; gpio++) {
        config_user_maybe |= (gpio_get(gpio) << (gpio - 13 + 1));
    }

    // Invert and mask to the five valid user switch bits
    config_user_maybe = (~config_user_maybe) & USER_SWITCH_VALID_MASK;
    if (config_user_maybe != (user_switches & USER_SWITCH_VALID_MASK)) {

        if (config_user_maybe != config_user_last) {
            debounce_user_cnt = 0;
            config_user_last = config_user_maybe;
        }
        debounce_user_cnt++;
        if (debounce_user_cnt >= debounce_user_done) {
            if (user_inputs_initialized) {
                // Song switch is edge-triggered: only register on stable
                // rising edges to avoid release chatter creating stale events.
                if ((config_user_maybe & USER_SWITCH_SONG_MASK) &&
                    !(user_switches & USER_SWITCH_SONG_MASK)) {
                    user_switch_flags |= USER_SWITCH_FLAG_SONG_TOGGLE_MASK;
                }

                // Pack power-up request is also rising-edge-triggered.
                if ((config_user_maybe & USER_SWITCH_PACK_PU_MASK) &&
                    !(user_switches & USER_SWITCH_PACK_PU_MASK)) {
                    user_switch_flags |= USER_SWITCH_FLAG_PACK_PU_REQ_MASK;
                } else if (!(config_user_maybe & USER_SWITCH_PACK_PU_MASK) &&
                           (user_switches & USER_SWITCH_PACK_PU_MASK)) {
                    user_switch_flags &= ~USER_SWITCH_FLAG_PACK_PU_REQ_MASK;
                }
            } else {
                user_inputs_initialized = true;
                user_switch_flags &= ~USER_SWITCH_FLAG_EDGE_EVENTS_MASK;
            }

            user_switches = config_user_maybe;
            debounce_user_cnt = 0;
        }
    } else {
        debounce_user_cnt = 0;
    }

    if (((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL1_MASK) ||
        (((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL_MASK) &&
         (config_dip_sw & DIP_HEAT_MASK))) {
        static bool fire_stable_pressed = false;
        static bool fire_last_sample = false;
        static uint8_t fire_stable_cnt = 0;
        const uint8_t fire_stable_done = 3;

        bool fire_sample_pressed = (gpio_get(15) == 0);
        if (fire_sample_pressed != fire_last_sample) {
            fire_last_sample = fire_sample_pressed;
            fire_stable_cnt = 0;
        } else if (fire_stable_cnt < fire_stable_done) {
            fire_stable_cnt++;
        }

        if ((fire_stable_cnt >= fire_stable_done) &&
            (fire_stable_pressed != fire_sample_pressed)) {
            fire_stable_pressed = fire_sample_pressed;
            if (!fire_stable_pressed) {
                if ((debounce_fire_cnt >= debounce_fire_found) &&
                    (debounce_fire_cnt <= debounce_fire_max)) {
                    user_switch_flags |= USER_SWITCH_FLAG_FIRE_TAP_MASK;
                }
                user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_HELD_MASK;
                debounce_fire_cnt = 0;
            } else {
                user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_TAP_MASK;
            }
        }

        if (fire_stable_pressed) {
            if (debounce_fire_cnt < 250) {
                debounce_fire_cnt++;
            }
            if (debounce_fire_cnt == debounce_fire_found) {
                user_switch_flags |= USER_SWITCH_FLAG_FIRE_HELD_MASK;
            } else if (debounce_fire_cnt == debounce_fire_max) {
                user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_MASK;
            }
        }
    } else {
        debounce_fire_cnt = 0;
        user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_MASK;
    }
}

// --- Switch state accessors ---
bool pack_pu_sw(void) { return (user_switches & USER_SWITCH_PACK_PU_MASK); }
bool pack_pu_req(void) {
    return (user_switch_flags & USER_SWITCH_FLAG_PACK_PU_REQ_MASK);
}
bool pu_sw(void) { return (user_switches & USER_SWITCH_PU_MASK); }
bool fire_sw(void) {
    return ((user_switches & USER_SWITCH_FIRE_MASK) &&
            !(user_switch_flags & USER_SWITCH_FLAG_FIRE_HELD_MASK));
}
bool fire_tap(void) {
    return (user_switch_flags & USER_SWITCH_FLAG_FIRE_TAP_MASK);
}
bool song_sw(void) { return (user_switches & USER_SWITCH_SONG_MASK); }
bool song_toggle(void) {
    return (user_switch_flags & USER_SWITCH_FLAG_SONG_TOGGLE_MASK);
}
bool vent_sw(void) { return (user_switches & USER_SWITCH_VENT_MASK); }
bool wand_standby_sw(void) { return (!pu_sw() && vent_sw()); }

// --- Flag clearing functions ---
void clear_fire_tap(void) {
    user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_MASK;
}
void clear_song_toggle(void) {
    user_switch_flags &= ~USER_SWITCH_FLAG_SONG_TOGGLE_MASK;
}
void clear_pack_pu_req(void) {
    user_switch_flags &= ~USER_SWITCH_FLAG_PACK_PU_REQ_MASK;
}

// --- Direct GPIO control ---
void nsignal_to_wandlights(bool autovent) {
    gpio_put(GPO_NBUSY_TO_WAND, autovent ? 0 : 1);
}
void vent_light_on(bool turn_on) {
    gpio_put(GPO_VENT_LIGHT, turn_on ? 1 : 0);
}
void mute_audio(void) { gpio_put(GPO_MUTE, 1); }
void unmute_audio(void) { gpio_put(GPO_MUTE, 0); }

/**
 * @brief Determines the current pack type based on DIP switch settings.
 * @return The configured `PackType`.
 */
PackType config_pack_type(void) {
    PackType pack_type = PACK_TYPE_SNAP_RED;
    if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL0_MASK) {
        pack_type = PACK_TYPE_FADE_RED;
    } else if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL1_MASK) {
        pack_type = PACK_TYPE_TVG_FADE;
    } else if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL_MASK) {
        if (config_dip_sw & DIP_HEAT_MASK) {
            pack_type = PACK_TYPE_AFTER_TVG;
        } else {
            pack_type = PACK_TYPE_AFTERLIFE;
        }
    }
    return pack_type;
}

/**
 * @brief Determines the cyclotron rotation direction.
 * @note Currently hardcoded to return 0 (clockwise).
 * @return 0 for clockwise, 1 for counter-clockwise.
 */
uint8_t config_cyclotron_dir(void) { return 0; }

#ifdef __cplusplus
}
#endif
