/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: monitors.c
 * Version 1.0.0
 * Date: 2025-07-01
 *
 * This file is part of the GhostLab42 Support for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 *
 * These are monitor procedures and their supporting procedures and variables.
 *    Song - song start and stop when switch is changed
 *    Hum - start the appropriate hum sound when needed
 *    Monster - play random sound and if fire is soon enough, play a response
 *    ADJ - timing updates based on the two ADJx adjustment potentiometers
 *    Mode - quick tap of the fire key causes a mode change for TVG packs
 *    Vent - perform a full vent sequence when a vent is requested
 *    Ring - Use ADJ1 to select the number of cyclotron LEDs (4 separate LEDs to 40 LED rings)
 */
#include "monitors.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "monster.h"
#include "heat.h"
#include "pack_config.h"
#include "addressable_LED_support.h"
#include "led_patterns.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "klystron_IO_support.h"
#include "sound_module.h"
#include "pack_state.h"
#include "party_sequences.h"

#define cyclotron_strip (*get_cyclotron_strip())


/** Track song state; MSB set when a song is playing. */
volatile uint8_t song;

bool song_is_playing(void) {
    return (song & 0x80);
}

/**
 * @brief Monitor the song switch and handle start/stop/party mode events.
 */
void song_monitor(void) {
    typedef enum {
        SONG_MONITOR_IDLE,
        SONG_MONITOR_DEBOUNCE,
        SONG_MONITOR_PLAYING,
        SONG_MONITOR_STOPPING
    } SongMonitorState;

    static SongMonitorState state = SONG_MONITOR_IDLE;
    static absolute_time_t debounce_timer;
    static uint8_t party_animation_index = 0; // 0 is off
    static bool last_fire_state = false;
    bool fire_now = fire_sw();
    bool tap_now = fire_tap();

    // If a song finishes on its own, reset state
    if (state == SONG_MONITOR_PLAYING && !sound_is_playing()) {
        if (party_mode_is_active()) {
            party_mode_stop();
        }
        party_animation_index = 0;
        state = SONG_MONITOR_IDLE;
    }

    switch (state) {
        case SONG_MONITOR_IDLE:
            if (song_toggle() && song_sw()) {
                clear_song_toggle();
                debounce_timer = get_absolute_time();
                state = SONG_MONITOR_DEBOUNCE;
            }
            break;

        case SONG_MONITOR_DEBOUNCE:
            if (absolute_time_diff_us(debounce_timer, get_absolute_time()) > 500 * 1000) {
                song = (song >= pack_song_count) ? 0x80 : 0x80 | (song + 1);
                sound_start_safely(96 + (song & 0x7f));
                party_mode_stop();
                party_animation_index = 0; // Reset party mode
                clear_song_toggle(); // ignore release edge
                state = SONG_MONITOR_PLAYING;
            }
            break;

        case SONG_MONITOR_PLAYING:
            if (song_toggle()) {
                clear_song_toggle();
                state = SONG_MONITOR_STOPPING;
            } else if (pack_state_get_state() == PS_OFF && ((fire_now && !last_fire_state) || tap_now)) {
                party_animation_index = (party_animation_index + 1) % (PARTY_ANIMATION_COUNT + 1); // 0=Off, 1=Rainbow, 2=Cylon, 3=Sparkle, 4=Beat
                if (party_animation_index == 0) {
                    party_mode_stop();
                } else {
                    party_mode_set_animation((party_animation_t)(party_animation_index - 1));
                }
                if (tap_now) {
                    clear_fire_tap();
                }
            }
            break;

        case SONG_MONITOR_STOPPING:
            sound_stop();
            if (party_mode_is_active()) {
                party_mode_stop();
            }
            party_animation_index = 0;
            song &= 0x7F; // Clear playing flag
            clear_song_toggle();
            state = SONG_MONITOR_IDLE;
            break;
    }

    if (tap_now) {
        clear_fire_tap();
    }
    last_fire_state = fire_now;
}

/**
 * @brief Start a sound and ensure playback begins, clearing any active song.
 *
 * @param sound_index Index of the sound to start.
 */
void sound_start_safely(uint8_t sound_index) {
    song &= 0x7F; // Clear the song playing flag
    if (sound_is_playing()) {
        sound_stop();
        do {
        } while (sound_is_playing());
    }
    sound_start(sound_index);
    do {
    } while (!sound_is_playing());
}

/**
 * @brief Start a sound and wait until completion.
 */
void sound_play_blocking(uint8_t sound_index, bool fire, bool shutdown) {
    sound_start_safely(sound_index);
    sound_wait_til_end(fire, shutdown);
}

/**
 * @brief Maintain hum playback when enabled via dip switch.
 */
void hum_monitor(void) {
    // add hum if hum dip switch is set
    if ( (config_dip_sw & DIP_HUM_MASK) && !sound_is_playing() ) {
        PackType type = config_pack_type();
        switch (pack_state_get_mode()) {
            case PACK_MODE_PROTON_STREAM: // Proton Pack
            case PACK_MODE_BOSON_DART: // Boson Dart
                if (type == PACK_TYPE_SNAP_RED) { // If the pack type is classic (red snap)
                    sound_start_safely(13); // Start the hum sound for a classic pack
                } else if (type == PACK_TYPE_FADE_RED) { // If the pack type fade red
                    sound_start_safely(60); // Start the hum sound for TVG fade
                } else if (type == PACK_TYPE_TVG_FADE) { // If the pack type is TVG
                    sound_start_safely(60); // Start the hum sound for TVG fade
                } else { // Afterlife & Afterlife TVG
                    sound_start_safely(120); // Start the hum sound for Afterlife pack
                }
                break; // Break out of the switch statement
            case PACK_MODE_SLIME_BLOWER: // Slime Blower
            case PACK_MODE_SLIME_TETHER: // Slime Tether
                sound_start_safely(25); // Start the hum sound for Slime Blower or Slime Tether
                break; // Break out of the switch statement
            case PACK_MODE_STASIS_STREAM: // Stasis Stream
            case PACK_MODE_SHOCK_BLAST: // Shock Blast
                sound_start_safely(34); // Start the hum sound for Stasis Stream or Shock Blast
                break; // Break out of the switch statement
            case PACK_MODE_OVERLOAD_PULSE: // Overload Pulse
            default: // Meson Collider
                sound_start_safely(44); // Start the hum sound for Overload Pulse or Meson Collider
                break; // Break out of the switch statement
        }
    }
}

/** Index of the last monster sound used. */
volatile uint8_t monster_sound_index = 0;

/**
 * @brief Manage random monster sounds and their responses.
 */
void monster_monitor(void) {
    uint8_t temp_random_index = 0;
    if (config_dip_sw & DIP_MONSTER_MASK) {
        if (song_is_playing()) {
            monster_timer = 0;
        } else if (monster_timer == 0) {
            monster_timer = 240 *
                ((rand() % (pack_monster_timing.max_seconds - pack_monster_timing.min_seconds)) +
                 pack_monster_timing.min_seconds);
            do {
                temp_random_index = rand() % pack_monster_sound_pair_count;
            } while (temp_random_index == monster_sound_index);
            monster_sound_index = temp_random_index;
        } else if (monster_timer == 3) {
            sound_play_blocking(pack_monster_sound_pairs[monster_sound_index][0], false, false);
            response_timer = pack_monster_timing.response_seconds * 240;
            monster_timer = 2;
        } else if ((monster_timer == 2) && (response_timer == 0)) {
            monster_timer = 0;
        } else if (monster_timer == 1) {
            sound_play_blocking(pack_monster_sound_pairs[monster_sound_index][1], false, false);
            monster_timer = 0;
        }
    } else {
        monster_clear();
    }
}

/**
 * @brief Convert an ADJ potentiometer reading to a pattern cycle time.
 *
 * @param adj_select Which ADJ input to sample.
 * @param heat_effect Apply heat-based speed adjustment when true.
 * @param apply_cy_speed Apply cyclotron speed multiplier when true.
 * @return Computed cycle duration in milliseconds.
 */
uint16_t adj_to_ms_cycle(uint8_t adj_select, bool heat_effect, bool apply_cy_speed) {
    uint32_t temp_calc = 0;
    temp_calc = pack_adj_min_ms +
                (((pack_adj_max_ms - pack_adj_min_ms) * (4095 - adj_pot[adj_select & 1])) >> 12);
    if (heat_effect) {
        temp_calc = temp_calc *
                    (256 - (1.5 * (temperature /
                                   (pack_heat_settings[config_pack_type()].start_autovent >> 7))));
        temp_calc = temp_calc >> 8;
    }
    if (apply_cy_speed) {
        temp_calc = (temp_calc * cy_speed_multiplier) >> 16;
    }

    temp_calc = (temp_calc >= pack_adj_max_ms) ? pack_adj_max_ms : temp_calc;
    temp_calc = (temp_calc <= pack_adj_min_ms >> 2) ? pack_adj_min_ms >> 2 : temp_calc;
    return temp_calc;
}
/**
 * @brief Update LED pattern speeds based on ADJ settings and pack heat.
 */
void adj_monitor(void) {
    bool heating_effect = config_dip_sw & DIP_HEAT_MASK; // is heat effect enabled?
    read_adj_potentiometers(true); // read the ADC and average the samples
    if (pc_pattern_num == PC_PATTERN_NORMAL) { // Only the normal pattern!
        pc_pattern_speed_update(adj_to_ms_cycle(PC_SPEED_DEFAULT, heating_effect, false));
    }
    /* Apply the cyclotron speed multiplier only during the firing cooldown for
       Afterlife packs so the powercell remains unaffected. */
    bool apply_speed = (pack_state_get_state() == PS_FIRE_COOLDOWN) &&
                       (config_pack_type() == PACK_TYPE_AFTERLIFE ||
                        config_pack_type() == PACK_TYPE_AFTER_TVG);
    cy_pattern_speed_update(adj_to_ms_cycle(PC_SPEED_DEFAULT, heating_effect, apply_speed));
}


/**
 * @brief Perform a major mode change with coordinated sounds and lights.
 *
 * @param cyclotron_pattern_base Base cyclotron pattern index.
 * @param first_sound Sound to play during drain.
 * @param second_sound Optional sound to play with fade-in.
 */
void mode_change_major(uint8_t cyclotron_pattern_base, uint8_t first_sound, uint8_t second_sound) {
    sound_start_safely(first_sound);
    pc_pattern_config(PC_PATTERN_DRAIN, 300, PC_CYCLE_INFINITE);
    if ( (config_pack_type() != PACK_TYPE_AFTERLIFE) && (config_pack_type() != PACK_TYPE_AFTER_TVG) ) {
        cy_pattern_config(CY_PATTERN_FADE_OUT, 300, CY_CYCLE_INFINITE);
        do {
            sleep_ms(20);
        } while (pc_pattern_running || cy_pattern_running || sound_is_playing());
    } else {
        do {
            sleep_ms(20);
        } while (pc_pattern_running || sound_is_playing());
    }
    update_pack_colors();
    pc_pattern_config(PC_PATTERN_NORMAL,
                      adj_to_ms_cycle(PC_SPEED_DEFAULT,false,false),
                      PC_CYCLE_INFINITE);
    do {
        sleep_ms(20);
    } while (sound_is_playing());
    if (second_sound != 0) {
        sound_start_safely(second_sound);
        if ( (config_pack_type() != PACK_TYPE_AFTERLIFE) && (config_pack_type() != PACK_TYPE_AFTER_TVG) ) {
            cy_pattern_config(CY_PATTERN_FADE_IN,1000, CY_CYCLE_INFINITE);
            do {
                sleep_ms(20);
            } while (cy_pattern_running || sound_is_playing());
        }
    }
    if ( (config_pack_type() != PACK_TYPE_AFTERLIFE) && (config_pack_type() != PACK_TYPE_AFTER_TVG) ) {
        cy_pattern_config(cyclotron_pattern_base + config_cyclotron_dir(),
                          adj_to_ms_cycle(PC_SPEED_DEFAULT,false,true),
                          CY_CYCLE_INFINITE);
    }
}

/**
 * @brief Monitor fire taps to cycle through available pack modes.
 */
void mode_monitor(void) {
    if (song_is_playing()) {
        return;
    }
    if (!fire_tap()) {
        return;
    }
    if ((config_pack_type() == PACK_TYPE_TVG_FADE) || (config_pack_type() == PACK_TYPE_AFTER_TVG)) {
        PackMode prev = pack_state_get_mode();
        PackMode next = prev + 1;
        switch (prev) {
        case PACK_MODE_PROTON_STREAM:
            pack_state_set_mode(next);
            sound_play_blocking(12, false, false);
            break;
        case PACK_MODE_BOSON_DART:
            pack_ctx.mode = next;
            mode_change_major(7, 23, 0);
            break;
        case PACK_MODE_SLIME_BLOWER:
            pack_state_set_mode(next);
            sound_play_blocking(12, false, false);
            break;
        case PACK_MODE_SLIME_TETHER:
            pack_ctx.mode = next;
            mode_change_major(5, 24, 32);
            break;
        case PACK_MODE_STASIS_STREAM:
            pack_state_set_mode(next);
            sound_play_blocking(12, false, false);
            break;
        case PACK_MODE_SHOCK_BLAST:
            pack_ctx.mode = next;
            mode_change_major(5, 33, 42);
            break;
        case PACK_MODE_OVERLOAD_PULSE:
            pack_state_set_mode(next);
            sound_play_blocking(12, false, false);
            break;
        default:
            pack_ctx.mode = PACK_MODE_PROTON_STREAM;
            mode_change_major(5, 43, 0);
            break;
        }
        cool_the_pack();
    }
    clear_fire_tap();
}


/** Saved powercell pattern while venting. */
volatile uint8_t recovery_pc_pattern_num;
/** Saved cyclotron pattern while venting. */
volatile uint8_t recovery_cy_pattern_num;

/** Save the current state of pack light patterns. */
void save_pack_light_states(void) {
    recovery_pc_pattern_num = pc_pattern_num;
    recovery_cy_pattern_num = cy_pattern_num;
}

/** Restore previously saved pack light patterns. */
void restore_pack_light_states(void) {
    pc_pattern_config(recovery_pc_pattern_num,
                      adj_to_ms_cycle(PC_SPEED_DEFAULT,false,false),
                      PC_CYCLE_INFINITE);
    if ( (config_pack_type() != PACK_TYPE_AFTERLIFE) && (config_pack_type() != PACK_TYPE_AFTER_TVG) ) {
        cy_pattern_config(recovery_cy_pattern_num,
                          adj_to_ms_cycle(PC_SPEED_DEFAULT,false,true),
                          CY_CYCLE_INFINITE);
    }
}

/**
 * @brief Run a full vent sequence with sound and lighting effects.
 */
void full_vent(void) {
    cool_the_pack();
    sound_start_safely(55);
    if ( (config_pack_type() == PACK_TYPE_AFTERLIFE) || (config_pack_type() == PACK_TYPE_AFTER_TVG) ) {
        fr_pattern_config(2,600,0);
    } else {
        fr_pattern_config(0,150,0);
    }
    pc_pattern_config(PC_PATTERN_SHUTDOWN, 3600, PC_CYCLE_INFINITE);
    if ( (config_pack_type() != PACK_TYPE_AFTERLIFE) && (config_pack_type() != PACK_TYPE_AFTER_TVG) ) {
        cy_pattern_config(CY_PATTERN_FADE_OUT, 3600, CY_CYCLE_INFINITE);
    }
    do {
        vent_light_on(true);
        sleep_ms(50);
        vent_light_on(false);
        sleep_ms(120);
    } while (vent_sw() || sound_is_playing());
    fr_pattern_stop(true);
}

/**
 * @brief Monitor the vent switch and trigger vent sequences or quotes.
 */
void vent_monitor(void) {
    static uint8_t slime_quote_counter = 0;
    if (vent_sw() && pu_sw()) {
        if ( (pack_state_get_mode() == PACK_MODE_SLIME_BLOWER) || (pack_state_get_mode() == PACK_MODE_SLIME_TETHER) ) {
            sound_play_blocking(150 + slime_quote_counter, false, false);
            slime_quote_counter = (slime_quote_counter+1) % pack_slime_quote_count;
            do {
                sleep_ms(10);
            } while (vent_sw());
        } else {
            save_pack_light_states();
            full_vent();
            restore_pack_light_states();
        }
    }
}

/**
 * @brief Adjust cyclotron LED ring size based on ADJ2 setting.
 */
void ring_monitor(void) {
    if (party_mode_is_active()) return;
    static uint8_t last_num_pixels = 0;
    static bool rainbow_active = false;
    static absolute_time_t rainbow_start;
    uint8_t current_num_pixels;

    read_adj_potentiometers(true);
    if      (adj_pot[1] < 0x180) {
        current_num_pixels = cyclotron_positions.classic[0][4];
        cyc_classic_index = 0;
    } else if (adj_pot[1] < 0x800) {
        current_num_pixels = cyclotron_positions.classic[1][4];
        cyc_classic_index = 1;
    } else if (adj_pot[1] < 0xE80) {
        current_num_pixels = cyclotron_positions.classic[2][4];
        cyc_classic_index = 2;
    } else {
        current_num_pixels = cyclotron_positions.classic[3][4];
        cyc_classic_index = 3;
    }

    if (current_num_pixels > cyclotron_strip.max_pixels) {
        current_num_pixels = cyclotron_strip.max_pixels;
    }

    // On first run, just initialize the value
    if (last_num_pixels == 0) {
        last_num_pixels = current_num_pixels;
    }

    if (current_num_pixels != last_num_pixels) {
        /* Blank the previous LEDs so removed pixels don't stay lit. */
        blank_cyclotron_string();
        cyclotron_strip.num_pixels = current_num_pixels;
        last_num_pixels = current_num_pixels;
        /* Display a rainbow for 5 seconds to give feedback. */
        cy_pattern_config(CY_PATTERN_DISPLAY_COUNT, 50, CY_CYCLE_INFINITE);
        rainbow_active = true;
        rainbow_start = get_absolute_time();
    }
    if (rainbow_active &&
        absolute_time_diff_us(rainbow_start, get_absolute_time()) > 5 * 1000 * 1000) {
        cy_pattern_stop(false);
        rainbow_active = false;
    }
    cyclotron_strip.num_pixels = current_num_pixels;
}

