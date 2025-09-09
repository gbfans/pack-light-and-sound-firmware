/**
 * @file party_sequences.cpp
 * @brief Manages the "Party Mode" animations.
 * @details This file implements several fun, non-canonical lighting patterns
 *          that can be activated by the user. These animations run on all
 *          three LED strips (Cyclotron, Powercell, Future) and respect the
 *          cyclotron's active LED count (`N`).
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "party_sequences.h"
#include "addressable_LED_support.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "animations.h"
#include "animation_controller.h"
#include "pack_state.h"
#include "Arduino.h"
#include <stdlib.h>

#include <FastLED.h>
#include <memory>

static party_animation_t current_animation = PARTY_ANIMATION_RAINBOW_FADE;
static bool party_mode_active = false;
static PartyModeState g_party_state;


// --- Main Party Mode Control ---

/**
 * @brief Runs the currently selected party mode animation.
 * @details This function is called on every main loop iteration. It updates
 *          the shared state for the party mode animations. The animations
 *          themselves are updated by their respective controllers.
 */
void party_mode_run(void) {
    if (!party_mode_active) return;

    // Update shared state based on current_animation
    switch (current_animation) {
        case PARTY_ANIMATION_RAINBOW_FADE:
            g_party_state.rainbow_hue++;
            break;
        case PARTY_ANIMATION_CYLON_SCANNER:
            if (millis() - g_party_state.sparkle_time > 3000) { // change every 3s
                g_party_state.sparkle_time = millis();
                g_party_state.cylon_eye_color = CHSV(rand() % 256, 255, 255);
                if (auto anim = g_powercell_controller.getCurrentAnimation()) anim->setColor(g_party_state.cylon_eye_color, 0);
                if (auto anim = g_cyclotron_controller.getCurrentAnimation()) anim->setColor(g_party_state.cylon_eye_color, 0);
                if (auto anim = g_future_controller.getCurrentAnimation()) anim->setColor(g_party_state.cylon_eye_color, 0);
            }
            break;
        case PARTY_ANIMATION_RANDOM_SPARKLE:
            if ((rand() % 100) < 30) {
                g_party_state.sparkle_strip_index = rand() % 3;
                g_party_state.sparkle_color = CHSV(rand() % 256, 255, 255);
            }
            break;
        case PARTY_ANIMATION_BEAT_METER:
            {
                static constexpr uint8_t BEAT_LIMIT_4_LED = 8;
                static constexpr uint8_t BEAT_LIMIT_DEFAULT = 2;
                uint8_t beat_limit = (g_cyclotron_led_count == 4) ? BEAT_LIMIT_4_LED : BEAT_LIMIT_DEFAULT;

                g_party_state.beat_meter_counter = (g_party_state.beat_meter_counter + 1) % beat_limit;
                if (g_party_state.beat_meter_counter == 0) {
                    g_party_state.beat_meter_level += g_party_state.beat_meter_direction;
                    if (g_party_state.beat_meter_direction > 0 && g_party_state.beat_meter_level >= g_party_state.beat_meter_max_level) {
                        g_party_state.beat_meter_level = g_party_state.beat_meter_max_level > 0 ? g_party_state.beat_meter_max_level - 1 : 0;
                        g_party_state.beat_meter_direction = -1;
                    } else if (g_party_state.beat_meter_direction < 0 && g_party_state.beat_meter_level <= 0) {
                        g_party_state.beat_meter_level = 0;
                        g_party_state.beat_meter_direction = 1;
                        g_party_state.beat_meter_color = CHSV(rand() % 256, 255, 255);
                        if (auto anim = g_powercell_controller.getCurrentAnimation()) anim->setColor(g_party_state.beat_meter_color, 0);
                        if (auto anim = g_cyclotron_controller.getCurrentAnimation()) anim->setColor(g_party_state.beat_meter_color, 0);
                        if (auto anim = g_future_controller.getCurrentAnimation()) anim->setColor(g_party_state.beat_meter_color, 0);
                    }
                }
            }
            break;
        case PARTY_ANIMATION_COUNT:
            break;
    }
}

/**
 * @brief Sets the active party mode animation.
 * @param animation The animation to activate.
 */
void party_mode_set_animation(party_animation_t animation) {
    if (animation >= PARTY_ANIMATION_COUNT) {
        party_mode_stop();
        return;
    }

    party_mode_stop(); // Stop current animations and clear LEDs

    current_animation = animation;
    party_mode_active = true;

    // Initialize shared state
    g_party_state = PartyModeState(); // Reset to default

    // Set up animations on all three controllers
    Animation* powercell_anim = nullptr;
    Animation* cyclotron_anim = nullptr;
    Animation* future_anim = nullptr;

    AnimationConfig pc_config = { g_powercell_leds, NUM_LEDS_POWERCELL };
    AnimationConfig cyc_config = { g_cyclotron_leds, g_cyclotron_led_count };
    AnimationConfig fut_config = { g_future_leds, NUM_LEDS_FUTURE };

    switch (animation) {
        case PARTY_ANIMATION_RAINBOW_FADE:
            powercell_anim = new PartyRainbowFadeAnimation(&g_party_state);
            cyclotron_anim = new PartyRainbowFadeAnimation(&g_party_state);
            future_anim = new PartyRainbowFadeAnimation(&g_party_state);
            break;
        case PARTY_ANIMATION_CYLON_SCANNER:
            {
                static constexpr uint16_t CYLON_SPEED_4_LED = 160; // ms
                static constexpr uint16_t CYLON_SPEED_DEFAULT = 40; // ms
                uint16_t speed = (g_cyclotron_led_count == 4) ? CYLON_SPEED_4_LED : CYLON_SPEED_DEFAULT;
                pc_config.speed = speed;
                cyc_config.speed = speed;
                fut_config.speed = speed;
                g_party_state.cylon_eye_color = CHSV(rand() % 256, 255, 255);
                pc_config.color = g_party_state.cylon_eye_color;
                cyc_config.color = g_party_state.cylon_eye_color;
                fut_config.color = g_party_state.cylon_eye_color;
                pc_config.bounce = true;
                cyc_config.bounce = true;
                fut_config.bounce = true;
                powercell_anim = new CylonAnimation();
                cyclotron_anim = new CylonAnimation();
                future_anim = new CylonAnimation();
                g_party_state.sparkle_time = millis(); // Initialize timer
            }
            break;
        case PARTY_ANIMATION_RANDOM_SPARKLE:
            powercell_anim = new PartyRandomSparkleAnimation(&g_party_state, 0);
            cyclotron_anim = new PartyRandomSparkleAnimation(&g_party_state, 1);
            future_anim = new PartyRandomSparkleAnimation(&g_party_state, 2);
            break;
        case PARTY_ANIMATION_BEAT_METER:
            {
                uint8_t max_level = NUM_LEDS_POWERCELL;
                if (g_cyclotron_led_count > max_level) max_level = g_cyclotron_led_count;
                if (NUM_LEDS_FUTURE > max_level) max_level = NUM_LEDS_FUTURE;
                g_party_state.beat_meter_max_level = max_level;
                g_party_state.beat_meter_color = CHSV(rand() % 256, 255, 255);

                pc_config.color = g_party_state.beat_meter_color;
                cyc_config.color = g_party_state.beat_meter_color;
                fut_config.color = g_party_state.beat_meter_color;

                powercell_anim = new BeatMeterAnimation(&g_party_state);
                cyclotron_anim = new BeatMeterAnimation(&g_party_state);
                future_anim = new BeatMeterAnimation(&g_party_state);
            }
            break;
        case PARTY_ANIMATION_COUNT:
            break;
    }

    if (powercell_anim) g_powercell_controller.play(std::unique_ptr<Animation>(powercell_anim), pc_config);
    if (cyclotron_anim) g_cyclotron_controller.play(std::unique_ptr<Animation>(cyclotron_anim), cyc_config);
    if (future_anim) g_future_controller.play(std::unique_ptr<Animation>(future_anim), fut_config);
}

/**
 * @brief Stops the currently active party mode animation.
 */
void party_mode_stop(void) {
    if (!party_mode_active) {
        return;
    }

    party_mode_active = false;
    g_powercell_controller.stop();
    g_cyclotron_controller.stop();
    g_future_controller.stop();

    fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
    fill_solid(g_cyclotron_leds, g_cyclotron_led_count, CRGB::Black);
    fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
}

/**
 * @brief Checks if party mode is currently active.
 * @return True if party mode is active, false otherwise.
 */
bool party_mode_is_active(void) {
    return party_mode_active;
}
