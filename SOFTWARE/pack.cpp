/**
 * @file pack.cpp
 * @brief Implements top-level pack behavior functions.
 * @details This file provides the main entry points for complex, coordinated
 *          pack behaviors like startup and shutdown sequences that involve
 *          multiple sounds and lighting effects.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack.h"
#include "addressable_LED_support.h"
#include "klystron_IO_support.h"
#include "led_patterns.h"
#include "monitors.h"
#include "pack_helpers.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "sound.h"
#include "sound_module.h"
#include "pack_state.h"
#include "pack_config.h"
#include "animations.h"
#include "action.h"
#include <memory>

/** Target speed ramp parameters for the Afterlife spin-up. */
static const uint32_t AFTERLIFE_RAMP_DURATION_MS = 6000;
static const uint32_t AFTERLIFE_RAMP_START_SPEED_X = 5;

/**
 * @brief Executes the main power-up sequence for the currently active pack type.
 * @details This function coordinates the sound and light animations for a full
 *          pack startup. The specific sounds and light patterns are determined
 *          by the pack type selected via DIP switches. It handles different
 *          startup logic for each pack variant (Classic, Fade, TVG, Afterlife).
 */
static void wait_for_animations_or_user() {
    do {
        sleep_ms(20);
        if (!pu_sw() && !pack_pu_sw() && !wand_standby_sw())
            break;
        if (fire_sw())
            break;
    } while (g_powercell_controller.isRunning() || g_cyclotron_controller.isRunning() || sound_is_playing());
}

void pack_combo_startup(void) {
    // Ensure colors are refreshed in case a previous shutdown left the
    // palettes altered.
    update_pack_colors();
    set_led_brightness(255, 0);
    /* Reset the cyclotron speed multiplier so each pack variant starts
     * from a neutral baseline regardless of the previous mode. */
    cy_speed_ramp_go(1 << 16, 0);
    cy_speed_ramp_update();
    if (config_pack_type() == PACK_TYPE_AFTERLIFE ||
        config_pack_type() == PACK_TYPE_AFTER_TVG) {
        // Start the Afterlife ramp from a small initial speed so the
        // cyclotron begins rotating immediately, then accelerate to the
        // screen-accurate top speed over a longer duration.
        cy_speed_ramp_go(AFTERLIFE_RAMP_START_SPEED_X << 16, 0);
        cy_speed_ramp_update();
        uint32_t target_speed = afterlife_target_speed_x();
        cy_speed_ramp_go(target_speed << 16, AFTERLIFE_RAMP_DURATION_MS);
    }

    switch (config_pack_type()) {
    case PACK_TYPE_SNAP_RED:
        sound_start_safely(10);
        {
            AnimationConfig pc_config;
            pc_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<ScrollAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true);
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.clockwise = (config_cyclotron_dir() == 0);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(std::make_unique<RotateAnimation>(), cy_config);
        }
        sound_wait_til_end(true, true);
        break;
    case PACK_TYPE_FADE_RED:
        sound_start_safely(10);
        {
            AnimationConfig pc_config;
            pc_config.speed = 4800;
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<WaterfallAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = 4800;
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(std::make_unique<FadeAnimation>(false), cy_config);
        }
        wait_for_animations_or_user();
        {
            AnimationConfig pc_config;
            pc_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<ScrollAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true);
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.clockwise = (config_cyclotron_dir() == 0);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            cy_config.fade_amount = 4;
            cy_config.steps = 64;
            g_cyclotron_controller.play(std::make_unique<RotateFadeAnimation>(), cy_config);
        }
        break;
    case PACK_TYPE_TVG_FADE:
        sound_start_safely(58);
        {
            AnimationConfig pc_config;
            pc_config.speed = 4800;
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<WaterfallAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = 4800;
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(std::make_unique<FadeAnimation>(false), cy_config);
        }
        wait_for_animations_or_user();
        {
            AnimationConfig pc_config;
            pc_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<ScrollAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true);
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.clockwise = (config_cyclotron_dir() == 0);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            cy_config.fade_amount = 4;
            cy_config.steps = 64;
            if (pack_state_get_mode() == PACK_MODE_SLIME_BLOWER ||
                pack_state_get_mode() == PACK_MODE_SLIME_TETHER) {
                g_cyclotron_controller.play(std::make_unique<SlimeAnimation>(), cy_config);
            } else {
                g_cyclotron_controller.play(std::make_unique<RotateFadeAnimation>(), cy_config);
            }
        }
        break;
    default: // Afterlife
        sound_start_safely(121);
        {
            AnimationConfig pc_config;
            pc_config.speed = 4800;
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<WaterfallAnimation>(), pc_config);

            AnimationConfig cy_config;
            cy_config.speed = 1000;
            cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
            cy_config.leds = g_cyclotron_leds;
            cy_config.num_leds = g_cyclotron_led_count;
            g_cyclotron_controller.play(std::make_unique<CylonAnimation>(), cy_config);
        }

        do {
            cy_speed_ramp_update();
            if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
                uint32_t speed = 1000;
                if (cy_speed_multiplier > 0) {
                    speed = speed * (1 << 16) / cy_speed_multiplier;
                }
                anim->setSpeed(speed, 0);
            }
            show_leds();
            sleep_ms(20);
            if (!pu_sw() && !pack_pu_sw() && !wand_standby_sw()) break;
            if (fire_sw()) break;
        } while (g_powercell_controller.isRunning() ||
                 ((config_pack_type() != PACK_TYPE_AFTERLIFE) &&
                  (config_pack_type() != PACK_TYPE_AFTER_TVG) &&
                  g_cyclotron_controller.isRunning()));

        {
            AnimationConfig pc_config;
            pc_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
            pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
            pc_config.leds = g_powercell_leds;
            pc_config.num_leds = NUM_LEDS_POWERCELL;
            g_powercell_controller.play(std::make_unique<ScrollAnimation>(), pc_config);
        }

        while (sound_is_playing()) {
            cy_speed_ramp_update();
            if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
                uint32_t speed = 1000;
                if (cy_speed_multiplier > 0) {
                    speed = speed * (1 << 16) / cy_speed_multiplier;
                }
                anim->setSpeed(speed, 0);
            }
            show_leds();
            if (!pu_sw() && !pack_pu_sw() && !wand_standby_sw()) break;
            if (fire_sw()) break;
            sleep_ms(20);
        }
        break;
    }
}

/**
 * @brief Plays a short power-up sound, typically for state transitions.
 * @details This is used when the pack is already partially on and is
 *          transitioning to a more active state, such as from wand standby
 *          to full idle.
 * @param afterlife_higher For Afterlife packs, if true, plays a higher-pitched
 *                         power-up tone. This parameter has no effect on other
 *                         pack types.
 */
void pack_short_powerup_sound(bool afterlife_higher) {
    PackType type = config_pack_type();
    uint8_t sound = pack_short_powerup_sounds[type];
    if (type >= PACK_TYPE_AFTERLIFE && afterlife_higher)
        sound = 125;
    sound_play_blocking(sound, true, true);
}

/**
 * @brief Executes the main power-down sequence for the currently active pack type.
 * @details This function coordinates the sound and light animations for a full
 *          pack shutdown. It uses a predefined sequence from the `pack_powerdown_sequences`
 *          configuration table.
 */
static void wait_for_sequence_end() {
    do {
        cy_speed_ramp_update();
        if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
            uint32_t speed = 1000;
            if (cy_speed_multiplier > 0) {
                speed = speed * (1 << 16) / cy_speed_multiplier;
            }
            anim->setSpeed(speed, 0);
        }
        show_leds();
        sleep_ms(20);
    } while (g_powercell_controller.isRunning() || g_cyclotron_controller.isRunning() || sound_is_playing());
    sleep_ms(10);
}

void pack_combo_powerdown(void) {
    const PackSequence* seq = &pack_powerdown_sequences[config_pack_type()];
    if (config_pack_type() == PACK_TYPE_AFTERLIFE ||
        config_pack_type() == PACK_TYPE_AFTER_TVG) {
        // Ramp down from the current speed over the fade duration instead of
        // jumping to a stop. The cyclotron handles its own fade-out so keep
        // the global brightness steady to avoid dimming the powercell during
        // shutdown.
        cy_speed_ramp_go(0, seq->cy_ms);
    }

    sound_start_safely(seq->sound);

    AnimationConfig pc_config;
    pc_config.speed = seq->pc_ms;
    pc_config.color = CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
    pc_config.leds = g_powercell_leds;
    pc_config.num_leds = NUM_LEDS_POWERCELL;
    if (seq->pc_pattern == PC_PATTERN_INSTANT_OFF) {
        g_powercell_controller.stop();
        fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
    } else {
        g_powercell_controller.play(std::make_unique<DrainAnimation>(), pc_config);
    }

    AnimationConfig cy_config;
    cy_config.speed = seq->cy_ms;
    cy_config.color = CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_config.leds = g_cyclotron_leds;
    cy_config.num_leds = g_cyclotron_led_count;

    if (config_pack_type() == PACK_TYPE_AFTERLIFE ||
        config_pack_type() == PACK_TYPE_AFTER_TVG) {
        g_cyclotron_controller.enqueue(std::make_unique<ChangeColorAction>(CRGB::Black, seq->cy_ms, QUADRATIC_OUT));
        g_cyclotron_controller.enqueue(std::make_unique<WaitAction>(seq->cy_ms));
        g_cyclotron_controller.enqueue(std::make_unique<CallbackAction>([]() { g_cyclotron_controller.stop(); }));
    } else {
        switch (seq->cy_pattern) {
        case CY_PATTERN_INSTANT_OFF:
            g_cyclotron_controller.stop();
            fill_solid(g_cyclotron_leds, g_cyclotron_led_count, CRGB::Black);
            break;
        case CY_PATTERN_FADE_OUT:
            g_cyclotron_controller.play(std::make_unique<FadeAnimation>(true), cy_config);
            break;
        case CY_PATTERN_RING_FADE_OUT:
            g_cyclotron_controller.play(std::make_unique<CylonFadeOutAnimation>(), cy_config);
            break;
        }
    }

    wait_for_sequence_end();

    if (config_pack_type() == PACK_TYPE_AFTERLIFE ||
        config_pack_type() == PACK_TYPE_AFTER_TVG) {
        // Ensure the multiplier is reset while the pack is off.
        cy_speed_ramp_go(0, 0);
        cy_speed_ramp_update();
    }

    // Reset LED brightness so off-state feedback like the ADJ1 rainbow
    // remains visible after shutdown.
    set_led_brightness(255, 0);
}
