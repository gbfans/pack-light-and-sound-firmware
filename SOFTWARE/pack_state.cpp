/**
 * @file pack_state.cpp
 * @brief Main pack state machine.
 * @details This file contains the core state machine that drives the pack's
 *          behavior. It transitions between states like OFF, IDLE, FIRE, etc.,
 *          and calls the appropriate functions for lighting, sound, and other
 *          effects based on the current state and user inputs.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack_state.h"
#include "Ramp.h"

#include "pack.h"
#include "pack_helpers.h"
#include "addressable_LED_support.h"
#include "klystron_IO_support.h"
#include "led_patterns.h"
#include "monitors.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "party_sequences.h"
#include "sound.h"
#include "sound_module.h"
#include "heat.h"
#include "monster.h"
#include "pack_config.h"
#include "pico/stdlib.h"
#include "animations.h"
#include <memory>

#define STANDALONE_USE false
#define AUTOVENT_MS_CYCLE 250
#define FEEDBACK_DURATION_MS 5000


/** Global pack state context. */
PackContext pack_ctx = {
    .startup_mode = PACK_MODE_PROTON_STREAM,
    .mode = PACK_MODE_PROTON_STREAM,
    .state = PS_OFF,
};

uint32_t cy_speed_multiplier = 1 << 16; // 16.16 fixed point for cyclotron speed control
static rampUnsignedLong cy_speed_ramp(cy_speed_multiplier);
static bool feedback_anim_needs_start = false;
static uint32_t feedback_end_time = 0;

void cy_speed_ramp_go(uint32_t target, unsigned long duration) {
    // Synchronize the ramp's starting point with the current multiplier so
    // mid-transition updates ramp smoothly from the present speed.
    cy_speed_multiplier = cy_speed_ramp.update();
    ramp_mode mode = (target > cy_speed_multiplier) ? QUADRATIC_IN : QUADRATIC_OUT;
    cy_speed_ramp.go(target, duration, mode);
}

void cy_speed_ramp_update(void) {
    cy_speed_multiplier = cy_speed_ramp.update();
}

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

void feedback_request(void) {
    AnimationConfig cy_config;
    cy_config.leds = g_cyclotron_leds;
    cy_config.num_leds = g_cyclotron_led_count;
    feedback_end_time = to_ms_since_boot(get_absolute_time()) + FEEDBACK_DURATION_MS;

    if (pack_state_get_state() != PS_FEEDBACK) {
        pack_state_set_state(PS_FEEDBACK);
        feedback_anim_needs_start = true;
    } else if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
        // Animation already running; update LED count and extend duration.
        static_cast<FeedbackRainbowAnimation*>(anim)->updateConfig(cy_config, FEEDBACK_DURATION_MS);
    } else {
        feedback_anim_needs_start = true;
    }
}

/**
 * @brief Main state machine processing function.
 * @details This function is called on every iteration of the main loop. It
 *          evaluates the current state and user inputs to determine and
 *          execute the correct pack behavior.
 */
void pack_state_process(void) {
    song_monitor();
    cy_speed_ramp_update();
    if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
        uint32_t speed = 0;
        if (cy_speed_multiplier > 0) {
            speed = (1000 * (1 << 16)) / cy_speed_multiplier;
        }
        anim->setSpeed(speed, 0);
    }
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
            if (!party_mode_is_active() &&
                !g_powercell_controller.isRunning() && !g_cyclotron_controller.isRunning()) {
                fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
                fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, CRGB::Black);
                fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
            }
            ring_monitor();
            show_leds();
            if (!STANDALONE_USE) {
                pack_state_set_mode(PACK_MODE_PROTON_STREAM);
            }
        } else {
            ring_monitor();
        }
        nsignal_to_wandlights(false);
        cool_the_pack();
        monster_clear();
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
    case PS_FEEDBACK:
        if (feedback_anim_needs_start) {
            AnimationConfig cy_config;
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(
                std::make_unique<FeedbackRainbowAnimation>(FEEDBACK_DURATION_MS),
                cy_config);
            feedback_anim_needs_start = false;
        }
        ring_monitor();
        if (to_ms_since_boot(get_absolute_time()) >= feedback_end_time) {
            g_cyclotron_controller.stop();
            pack_state_set_state(PS_OFF);
        } else if (!g_cyclotron_controller.isRunning()) {
            pack_state_set_state(PS_OFF);
        }
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
            if ((config_pack_type() == PACK_TYPE_AFTERLIFE ||
                 config_pack_type() == PACK_TYPE_AFTER_TVG) &&
                next == PS_FIRE) {
                uint32_t base = afterlife_target_speed_x();
                uint32_t high = (base * 5) / 4;
                uint16_t start_autovent = pack_heat_settings[config_pack_type()].start_autovent;
                uint32_t remaining = (temperature < start_autovent) ? (start_autovent - temperature) : 0;
                uint32_t duration = remaining * pack_isr_interval_ms;
                cy_speed_ramp_go(high << 16, duration);
            }
        } else {
            hum_monitor();
            monster_monitor();
            adj_monitor();
            mode_monitor();
            vent_monitor();
        }
        break;
    case PS_FIRE_COOLDOWN: {
        hum_monitor();
        adj_monitor();
        // The fire cooldown sequence is now managed by the AnimationController's action queue.
        // This state is now just a waiting state. We could add logic here to
        // interrupt the sequence if needed.
    } break;
    case PS_SLIME_FIRE:
        if (!fire_sw()) {
            pack_state_set_state(PS_IDLE);
            fire_department(1);
            g_future_controller.stop();
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
                const uint32_t slowdown_duration = 1000;
                const uint32_t speedup_duration = 4000;
                uint32_t target = afterlife_target_speed_x();
                const uint32_t max_slow = target << 15;

                g_cyclotron_controller.enqueue(std::make_unique<CallbackAction>([=]() { cy_speed_ramp_go(max_slow, slowdown_duration); }));
                g_cyclotron_controller.enqueue(std::make_unique<WaitAction>(slowdown_duration));
                g_cyclotron_controller.enqueue(std::make_unique<CallbackAction>([=]() {
                    cy_speed_ramp_go(target << 16, speedup_duration);
                }));
                g_cyclotron_controller.enqueue(std::make_unique<WaitAction>(speedup_duration));
                g_cyclotron_controller.enqueue(std::make_unique<CallbackAction>([]() {
                    sound_stop();
                    hum_monitor();
                    AnimationConfig config;
                    config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
                    config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
                    config.leds = g_powercell_leds;
                    config.num_leds = NUM_LEDS_POWERCELL;
                    g_powercell_controller.play(std::make_unique<ScrollAnimation>(), config);
                    pack_state_set_state(PS_IDLE);
                }));

            } else {
                pack_state_set_state(PS_IDLE);
            }
            fire_department(1);
            g_future_controller.stop();
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
    case PS_AUTOVENT: {
        AnimationConfig pc_config;
        pc_config.speed = AUTOVENT_MS_CYCLE;
        pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
        pc_config.leds = g_powercell_leds;
        pc_config.num_leds = NUM_LEDS_POWERCELL;
        g_powercell_controller.play(std::make_unique<StrobeAnimation>(), pc_config);

        if ((config_pack_type() != PACK_TYPE_AFTERLIFE) &&
            (config_pack_type() != PACK_TYPE_AFTER_TVG)) {
            AnimationConfig cy_config;
            cy_config.speed = AUTOVENT_MS_CYCLE;
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(std::make_unique<StrobeAnimation>(), cy_config);
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
        pack_state_set_state(PS_IDLE);
        hum_monitor();
        while (fire_sw()) {
            sleep_ms(50);
        }
        clear_song_toggle();
        break;
    }
    default:
        break;
    }
}
