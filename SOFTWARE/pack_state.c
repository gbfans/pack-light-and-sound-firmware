/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack_state.h"

#include "pack.h"
#include "addressable_LED_support.h"
#include "klystron_IO_support.h"
#include "led_patterns.h"
#include "monitors.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "party_sequences.h"
#include "sound.h"
#include "sound_module.h"
#include "heat.h"
#include "monster.h"
#include "pack_config.h"
#include "pico/stdlib.h"

#define STANDALONE_USE false
#define AUTOVENT_MS_CYCLE 250

/** Global pack state context. */
PackContext pack_ctx = {
    .startup_mode = PACK_MODE_PROTON_STREAM,
    .mode = PACK_MODE_PROTON_STREAM,
    .state = PS_OFF,
};

uint32_t cy_speed_multiplier = 1 << 16; // 16.16 fixed point for cyclotron speed control

void pack_state_init(void) {
    pack_ctx.mode = ((config_pack_type() == PACK_TYPE_TVG_FADE) ||
                     (config_pack_type() == PACK_TYPE_AFTER_TVG))
                        ? pack_ctx.startup_mode
                        : PACK_MODE_PROTON_STREAM;
    pack_ctx.state = PS_OFF;
    update_pack_colors();
    clear_fire_tap();
    clear_pack_pu_req();
    song = pack_song_count;
}

void pack_state_set_mode(PackMode mode) {
    pack_ctx.mode = mode;
    update_pack_colors();
}

PackMode pack_state_get_mode(void) { return pack_ctx.mode; }

void pack_state_set_state(PackState state) { pack_ctx.state = state; }

PackState pack_state_get_state(void) { return pack_ctx.state; }

void pack_state_process(void) {
    song_monitor();
    if (pack_ctx.state != PS_OFF && party_mode_is_active()) {
        party_mode_stop();
    }
    firing_now = (((pack_ctx.state == PS_FIRE) ||
                   (pack_ctx.state == PS_SLIME_FIRE) ||
                   (pack_ctx.state == PS_OVERHEAT)) &&
                  fire_sw());
    switch (pack_ctx.state) {
    case PS_OFF:
        if (!song_is_playing()) {
            ring_monitor();
            if (!party_mode_is_active() &&
                !pc_pattern_running && !cy_pattern_is_running()) {
                blank_all_strings();
            }
            if (!STANDALONE_USE) {
                pack_state_set_mode(PACK_MODE_PROTON_STREAM);
            }
        }
        nsignal_to_wandlights(false);
        cool_the_pack();
        monster_clear();
        ring_monitor();
        if (!STANDALONE_USE) {
            pack_state_set_mode(PACK_MODE_PROTON_STREAM);
        }
        if (!song_is_playing() && pu_sw()) {
            if (party_mode_is_active()) party_mode_stop();
            pack_state_set_state(PS_IDLE);
            pack_combo_startup();
        } else if (!song_is_playing() && pack_pu_req()) {
            if (party_mode_is_active()) party_mode_stop();
            clear_pack_pu_req();
            pack_state_set_state(PS_PACK_STANDBY);
            pack_combo_startup();
        } else if (!song_is_playing() && wand_standby_sw()) {
            if (party_mode_is_active()) party_mode_stop();
            pack_state_set_state(PS_WAND_STANDBY);
            pack_combo_startup();
        }
        clear_fire_tap();
        break;
    case PS_PACK_STANDBY:
        monster_clear();
        if (!song_is_playing() && pu_sw()) {
            pack_state_set_state(PS_IDLE);
            pack_short_powerup_sound(false);
        } else if (!song_is_playing() && wand_standby_sw()) {
            pack_state_set_state(PS_WAND_STANDBY);
            pack_short_powerup_sound(false);
        } else if (!song_is_playing() && !pack_pu_sw()) {
            pack_state_set_state(PS_OFF);
            pack_combo_powerdown();
        } else {
            hum_monitor();
            adj_monitor();
        }
        clear_fire_tap();
        break;
    case PS_WAND_STANDBY:
        monster_clear();
        if (!song_is_playing() && pu_sw()) {
            pack_state_set_state(PS_IDLE);
            pack_short_powerup_sound(true);
        } else if (!song_is_playing() && !wand_standby_sw()) {
            if (pack_pu_req()) {
                clear_pack_pu_req();
                pack_state_set_state(PS_PACK_STANDBY);
                sound_play_blocking(59, false, false);
            } else {
                pack_state_set_state(PS_OFF);
                pack_combo_powerdown();
            }
        } else {
            hum_monitor();
            adj_monitor();
        }
        clear_fire_tap();
        break;
    case PS_IDLE:
        if (!song_is_playing() && !pu_sw()) {
            if (pack_pu_req()) {
                pack_state_set_state(PS_PACK_STANDBY);
                sound_play_blocking(59, false, false);
            } else if (wand_standby_sw()) {
                pack_state_set_state(PS_WAND_STANDBY);
                sound_play_blocking(59, false, false);
            } else {
                pack_state_set_state(PS_OFF);
                pack_combo_powerdown();
            }
        } else if (!song_is_playing() && fire_sw()) {
            PackMode mode = pack_state_get_mode();
            PackState next = (((config_pack_type() == PACK_TYPE_TVG_FADE) ||
                               (config_pack_type() == PACK_TYPE_AFTER_TVG)) &&
                              (mode == PACK_MODE_SLIME_BLOWER ||
                               mode == PACK_MODE_SLIME_TETHER))
                                 ? PS_SLIME_FIRE
                                 : PS_FIRE;
            pack_state_set_state(next);
            monster_fire();
            fire_department(0);
        } else {
            hum_monitor();
            monster_monitor();
            adj_monitor();
            mode_monitor();
            vent_monitor();
        }
        break;
    case PS_FIRE_COOLDOWN:
        {
            static absolute_time_t cooldown_start_time;
            static bool cooldown_init = true;

            if (cooldown_init) {
                cooldown_start_time = get_absolute_time();
                cooldown_init = false;
            }

            int64_t elapsed_us = absolute_time_diff_us(cooldown_start_time, get_absolute_time());
            uint32_t elapsed_ms = elapsed_us / 1000;

            const uint32_t slowdown_duration = 500;
            const uint32_t speedup_duration = 1500;
            const uint32_t total_duration = slowdown_duration + speedup_duration;
            const uint32_t max_slow = 2 << 16; // 50% speed (double the cycle time)

            if (elapsed_ms < slowdown_duration) {
                cy_speed_multiplier = (1 << 16) + ((max_slow - (1 << 16)) * elapsed_ms / slowdown_duration);
            } else if (elapsed_ms < total_duration) {
                cy_speed_multiplier = max_slow - ((max_slow - (1 << 16)) * (elapsed_ms - slowdown_duration) / speedup_duration);
            } else {
                cy_speed_multiplier = 1 << 16;
                cooldown_init = true;
                sound_stop();
                hum_monitor();
                pc_pattern_config(PC_PATTERN_NORMAL,
                                  adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false),
                                  PC_CYCLE_INFINITE);
                pack_state_set_state(PS_IDLE);
            }

            adj_monitor();
        }
        break;
    case PS_SLIME_FIRE:
        if (!fire_sw()) {
            pack_state_set_state(PS_IDLE);
            fire_department(1);
            clear_fire_tap();
            while (fire_sw()) {
                sleep_ms(50);
            }
        } else if (temperature >=
                   pack_heat_settings[config_pack_type()].start_autovent) {
            pack_state_set_state(PS_IDLE);
            fire_department(3);
            sound_wait_til_end(false, false);
            hum_monitor();
            cool_the_pack();
            adj_monitor();
            while (fire_sw()) {
                sleep_ms(50);
            }
            clear_song_toggle();
        } else if (!sound_is_playing()) {
            fire_department(0);
        }
        adj_monitor();
        break;
    case PS_FIRE:
        if (!fire_sw()) {
            if (config_pack_type() == PACK_TYPE_AFTERLIFE || config_pack_type() == PACK_TYPE_AFTER_TVG) {
                pack_state_set_state(PS_FIRE_COOLDOWN);
            } else {
                pack_state_set_state(PS_IDLE);
            }
            fire_department(1);
            clear_fire_tap();
        } else if (temperature >=
                   pack_heat_settings[config_pack_type()].start_beep) {
            if (config_dip_sw & DIP_HEAT_MASK) {
                pack_state_set_state(PS_OVERHEAT);
                fire_department(2);
            } else {
                cool_the_pack();
            }
        } else if (!sound_is_playing()) {
            fire_department(0);
        }
        adj_monitor();
        break;
    case PS_OVERHEAT:
        if (fire_sw()) {
            if (temperature >=
                pack_heat_settings[config_pack_type()].start_autovent) {
                pack_state_set_state(PS_AUTOVENT);
                fire_department(3);
            } else if (!sound_is_playing()) {
                fire_department(2);
            }
        } else {
            pack_state_set_state(PS_OVERHEAT_BEEP);
            fire_department(3);
            sound_wait_til_end(false, false);
            clear_fire_tap();
        }
        adj_monitor();
        break;
    case PS_OVERHEAT_BEEP:
        if (temperature <
            pack_heat_settings[config_pack_type()].start_beep) {
            pack_state_set_state(PS_IDLE);
        } else if (fire_sw()) {
            pack_state_set_state(PS_OVERHEAT);
            fire_department(0);
            fire_department(2);
            clear_fire_tap();
        } else {
            sound_play_blocking(53, false, false);
        }
        adj_monitor();
        break;
    case PS_AUTOVENT:
        save_pack_light_states();
        pc_pattern_config(PC_PATTERN_OVERHEAT, AUTOVENT_MS_CYCLE, PC_CYCLE_INFINITE);
        if ((config_pack_type() != PACK_TYPE_AFTERLIFE) &&
            (config_pack_type() != PACK_TYPE_AFTER_TVG)) {
            cy_pattern_config(CY_PATTERN_VENT_FADE, AUTOVENT_MS_CYCLE, CY_CYCLE_INFINITE);
        }
        if ((!STANDALONE_USE) &&
            ((config_pack_type() == PACK_TYPE_TVG_FADE) ||
             (config_pack_type() == PACK_TYPE_AFTER_TVG))) {
            sleep_align_wandlights();
        }
        nsignal_to_wandlights(true);
        sound_wait_til_end(false, false);
        sound_play_blocking(54, false, false);
        if ((!STANDALONE_USE) &&
            ((config_pack_type() != PACK_TYPE_TVG_FADE) &&
             (config_pack_type() != PACK_TYPE_AFTER_TVG))) {
            sleep_align_wandlights();
        }
        full_vent();
        nsignal_to_wandlights(false);
        restore_pack_light_states();
        pack_state_set_state(PS_IDLE);
        hum_monitor();
        while (fire_sw()) {
            sleep_ms(50);
        }
        clear_song_toggle();
        break;
    default:
        break;
    }
}
