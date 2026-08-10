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

// === FIRE input timing ===
//
// Everything below is counted in ISR ticks of PACK_ISR_INTERVAL_MS, so a
// measured pulse width is only accurate to within one tick either way.

/** @brief Stable samples required before a fire edge is accepted (12 ms). */
static const uint16_t FIRE_DEBOUNCE_TICKS = PACK_ISR_TICKS(12);
/** @brief Shortest pulse accepted as a deliberate tap (20 ms). */
static const uint16_t FIRE_TAP_MIN_TICKS = PACK_ISR_TICKS(20);
/** @brief Tap window when a wand lights board drives the fire line. */
static const uint16_t FIRE_TAP_WINDOW_WAND_TICKS = PACK_ISR_TICKS(140);
/** @brief Tap window when a plain switch drives the fire line. */
static const uint16_t FIRE_TAP_WINDOW_STANDALONE_TICKS = PACK_ISR_TICKS(300);

// --- Wand lights link detection ---
//
// There is no dedicated wand-present input, so the link is identified from
// the shape of the pulses arriving on the fire line. A wand lights board
// never passes the raw button through:
//
//   * a press of the wand's FIRE button is stretched to at least 180 ms
//   * a press of the wand's EAR button emits a fixed 100 ms pulse
//
// So a wand can only ever produce a pulse of about 100 ms or one of 180 ms
// and up; nothing in between, and nothing shorter. A switch wired straight to
// the FIRE input has no such shaping and lands wherever the user's thumb puts
// it - typically well above 140 ms, which is exactly why a 140 ms window is
// unusable standalone.
//
// A completed pulse is therefore classed as:
//
//   76..124 ms  only the wand's ear pulse lands here -> wand attached
//   128..164 ms a wand cannot generate this width    -> standalone
//   otherwise   ambiguous, leaves the link state alone
//
// Until there is evidence either way a wand is assumed, because mistaking a
// wand's 180 ms fire pulse for a mode change desynchronises the pack from the
// wand, which is far worse than a standalone tap firing once before the link
// is identified.

/** @brief Pulse width range matching the wand's fixed ear pulse. */
static const uint16_t FIRE_WAND_EAR_MIN_TICKS = PACK_ISR_TICKS(76);
static const uint16_t FIRE_WAND_EAR_MAX_TICKS = PACK_ISR_TICKS(124);
/** @brief Pulse width range no wand lights board can produce. */
static const uint16_t FIRE_WAND_GAP_MIN_TICKS = PACK_ISR_TICKS(128);
static const uint16_t FIRE_WAND_GAP_MAX_TICKS = PACK_ISR_TICKS(164);
/** @brief Contrary pulses needed to overturn an established link state. */
static const uint8_t FIRE_LINK_FLIP_COUNT = 2;

/** @brief What is currently believed to be driving the fire line. */
static volatile FireLinkState fire_link = FIRE_LINK_UNKNOWN;

/**
 * @brief Feed a completed fire pulse width into the link detector.
 * @param width_ticks Measured width of the pulse in ISR ticks.
 */
static void classify_fire_pulse(uint16_t width_ticks) {
    static uint8_t contrary_pulses = 0;
    FireLinkState evidence = FIRE_LINK_UNKNOWN;

    if ((width_ticks >= FIRE_WAND_EAR_MIN_TICKS) &&
        (width_ticks <= FIRE_WAND_EAR_MAX_TICKS)) {
        evidence = FIRE_LINK_WAND;
    } else if ((width_ticks >= FIRE_WAND_GAP_MIN_TICKS) &&
               (width_ticks <= FIRE_WAND_GAP_MAX_TICKS)) {
        evidence = FIRE_LINK_STANDALONE;
    }

    if (evidence == FIRE_LINK_UNKNOWN) {
        return;
    }
    if (fire_link == FIRE_LINK_UNKNOWN) {
        fire_link = evidence;
        contrary_pulses = 0;
    } else if (evidence == fire_link) {
        contrary_pulses = 0;
    } else if (++contrary_pulses >= FIRE_LINK_FLIP_COUNT) {
        // Only overturn a settled link after repeated disagreement so a single
        // oddly timed press cannot flip the timing out from under the user.
        fire_link = evidence;
        contrary_pulses = 0;
    }
}

/** @brief Tap window currently in force, in ISR ticks. */
static uint16_t fire_tap_window_ticks(void) {
    return (fire_link == FIRE_LINK_STANDALONE)
               ? FIRE_TAP_WINDOW_STANDALONE_TICKS
               : FIRE_TAP_WINDOW_WAND_TICKS;
}

/**
 * @brief ISR-based debounce and classification of the FIRE input.
 * @details In TVG modes a press is ambiguous until the tap window has passed:
 *          a pulse that ends inside the window is a mode change request and a
 *          pulse still active at the end of it is a fire request. Firing is
 *          therefore held off until the window expires, and the tap event is
 *          only raised once the release has been debounced. Every other pack
 *          type has no mode cycling, so firing starts as soon as the contact
 *          is debounced.
 */
static void check_fire_switch_isr(void) {
    static bool fire_stable_pressed = false;
    static bool fire_last_sample = false;
    static uint16_t fire_stable_cnt = 0;
    static uint16_t fire_press_ticks = 0;

    const bool tvg = config_pack_is_tvg();
    const uint16_t tap_window = fire_tap_window_ticks();

    bool fire_sample_pressed = (gpio_get(GPI_FIRE) == 0);
    if (fire_sample_pressed != fire_last_sample) {
        fire_last_sample = fire_sample_pressed;
        fire_stable_cnt = 0;
    } else if (fire_stable_cnt < FIRE_DEBOUNCE_TICKS) {
        fire_stable_cnt++;
    }

    // Count every sample the contact has been seen closed for. The counter is
    // frozen the moment the raw input opens, so a width can never creep past
    // the tap window while the release is still being debounced. That keeps
    // "released inside the window" and "still held at the end of it" mutually
    // exclusive.
    if (fire_stable_pressed && fire_sample_pressed &&
        (fire_press_ticks < UINT16_MAX)) {
        fire_press_ticks++;
    }

    if ((fire_stable_cnt >= FIRE_DEBOUNCE_TICKS) &&
        (fire_stable_pressed != fire_sample_pressed)) {
        fire_stable_pressed = fire_sample_pressed;
        if (fire_stable_pressed) {
            // Account for the samples that went into accepting the press:
            // this one plus the FIRE_DEBOUNCE_TICKS before it.
            fire_press_ticks = FIRE_DEBOUNCE_TICKS + 1;
            user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_TAP_MASK;
            user_switches |= USER_SWITCH_FIRE_MASK;
        } else {
            if (tvg) {
                if ((fire_press_ticks >= FIRE_TAP_MIN_TICKS) &&
                    (fire_press_ticks <= tap_window)) {
                    user_switch_flags |= USER_SWITCH_FLAG_FIRE_TAP_MASK;
                }
                // Only TVG presses are useful evidence: they are the ones made
                // while trying to change mode. Short bursts of firing in the
                // other pack types would just muddy the classification.
                classify_fire_pulse(fire_press_ticks);
            }
            // Drop the press straight away. Leaving it set would let fire_sw()
            // read true for the rest of the release debounce and turn a mode
            // change into a burst of firing.
            user_switches &= ~USER_SWITCH_FIRE_MASK;
            fire_press_ticks = 0;
        }
    }

    // Re-evaluated every tick rather than latched on an edge so that the main
    // loop clearing flags, or the pack type changing mid-press, cannot leave
    // the gate stuck in the wrong position.
    if (!tvg) {
        user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_MASK;
    } else if (fire_stable_pressed && (fire_press_ticks <= tap_window)) {
        user_switch_flags |= USER_SWITCH_FLAG_FIRE_HELD_MASK;
    } else {
        user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_HELD_MASK;
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
    static bool user_inputs_initialized = false;
    const uint8_t debounce_user_done = 15;

    uint8_t config_user_maybe = gpio_get(11);
    for (int gpio = 13; gpio <= 16; gpio++) {
        config_user_maybe |= (gpio_get(gpio) << (gpio - 13 + 1));
    }

    // Invert and mask to the user switch bits debounced here. FIRE is handled
    // by check_fire_switch_isr() below, which owns that bit.
    config_user_maybe = (~config_user_maybe) & USER_SWITCH_DEBOUNCED_MASK;
    if (config_user_maybe != (user_switches & USER_SWITCH_DEBOUNCED_MASK)) {

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

            user_switches =
                (user_switches & USER_SWITCH_FIRE_MASK) | config_user_maybe;
            debounce_user_cnt = 0;
        }
    } else {
        debounce_user_cnt = 0;
    }

    check_fire_switch_isr();
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
    // Only the tap event is cleared here. The held-off flag tracks the state
    // of the button itself and is owned by check_fire_switch_isr().
    user_switch_flags &= ~USER_SWITCH_FLAG_FIRE_TAP_MASK;
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
 * @brief Reports whether the configured pack type cycles TVG weapon modes.
 * @return True for the TVG and Afterlife TVG pack types.
 */
bool config_pack_is_tvg(void) {
    PackType pack_type = config_pack_type();
    return ((pack_type == PACK_TYPE_TVG_FADE) ||
            (pack_type == PACK_TYPE_AFTER_TVG));
}

/**
 * @brief Determines the cyclotron rotation direction.
 * @note Currently hardcoded to return 0 (clockwise).
 * @return 0 for clockwise, 1 for counter-clockwise.
 */
uint8_t config_cyclotron_dir(void) { return 0; }

/**
 * @brief Reports what the fire line detector currently believes is attached.
 */
FireLinkState fire_link_state(void) { return fire_link; }

/**
 * @brief Longest fire pulse currently treated as a TVG mode change request.
 */
uint16_t fire_tap_window_ms(void) {
    return (uint16_t)(fire_tap_window_ticks() * PACK_ISR_INTERVAL_MS);
}

#ifdef __cplusplus
}
#endif
