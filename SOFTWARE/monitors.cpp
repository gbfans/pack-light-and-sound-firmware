/**
 * @file monitors.cpp
 * @brief High-level monitors for inputs and events.
 * @details This file contains the monitor functions that are called on each
 *          main loop iteration to check for input changes (switches, pots),
 *          and manage ongoing events like songs, hum, and the monster mode.
 *          It is responsible for translating hardware inputs into high-level
 *          state changes and light/sound commands.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */
#include "monitors.h"
#include "action.h"
#include "addressable_LED_support.h"
#include "animation_controller.h"
#include "animations.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "heat.h"
#include "klystron_IO_support.h"
#include "led_patterns.h"
#include "monster.h"
#include "pack_config.h"
#include "pack_state.h"
#include "party_sequences.h"
#include "pico/stdlib.h"
#include "powercell_sequences.h"
#include "sound_module.h"
#include <stdlib.h>

/** Maximum time to wait for mode change effects (ms). */
static const uint32_t MODE_CHANGE_TIMEOUT_MS = 5000;

/** Track song state; MSB set when a song is playing. */
volatile uint8_t song;

bool song_is_playing(void) { return (song & 0x80); }

/**
 * @brief Monitor the song switch and handle start/stop/party mode events.
 * @details This function acts as a state machine for the song switch. It
 *          debounces the switch, starts/stops songs, and handles the special
 *          entry sequence for Party Mode (song switch on + fire button tap
 *          while pack is off).
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
    if (absolute_time_diff_us(debounce_timer, get_absolute_time()) >
        500 * 1000) {
      song = (song >= pack_song_count) ? 0x80 : 0x80 | (song + 1);
      sound_start_safely(96 + (song & 0x7f));
      party_mode_stop();
      party_animation_index = 0; // Reset party mode
      clear_song_toggle();       // ignore release edge
      state = SONG_MONITOR_PLAYING;
    }
    break;

  case SONG_MONITOR_PLAYING:
    if (song_toggle()) {
      clear_song_toggle();
      state = SONG_MONITOR_STOPPING;
    } else if (pack_state_get_state() == PS_OFF &&
               ((fire_now && !last_fire_state) || tap_now)) {
      party_animation_index =
          (party_animation_index + 1) %
          (PARTY_ANIMATION_COUNT +
           1); // 0=Off, 1=Rainbow, 2=Cylon, 3=Sparkle, 4=Beat
      if (party_animation_index == 0) {
        party_mode_stop();
      } else {
        party_mode_set_animation(
            (party_animation_t)(party_animation_index - 1));
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
  // Ensure any LED DMA transfers have completed to avoid contention when
  // communicating with the sound module.
  if (sound_is_playing()) {
    sound_stop();
    absolute_time_t start = get_absolute_time();
    while (sound_is_playing()) {
      if (absolute_time_diff_us(start, get_absolute_time()) >
          MODE_CHANGE_TIMEOUT_MS * 1000) {
        break;
      }
      tight_loop_contents();
    }
  }
  sound_start(sound_index);
  absolute_time_t start = get_absolute_time();
  while (!sound_is_playing()) {
    if (absolute_time_diff_us(start, get_absolute_time()) >
        MODE_CHANGE_TIMEOUT_MS * 1000) {
      break;
    }
    tight_loop_contents();
  }
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
  if ((config_dip_sw & DIP_HUM_MASK) && !sound_is_playing()) {
    PackType type = config_pack_type();
    switch (pack_state_get_mode()) {
    case PACK_MODE_PROTON_STREAM: // Proton Pack
    case PACK_MODE_BOSON_DART:    // Boson Dart
      if (type ==
          PACK_TYPE_SNAP_RED) { // If the pack type is classic (red snap)
        sound_start_safely(13); // Start the hum sound for a classic pack
      } else if (type == PACK_TYPE_FADE_RED) { // If the pack type fade red
        sound_start_safely(60); // Start the hum sound for TVG fade
      } else if (type == PACK_TYPE_TVG_FADE) { // If the pack type is TVG
        sound_start_safely(60);  // Start the hum sound for TVG fade
      } else {                   // Afterlife & Afterlife TVG
        sound_start_safely(120); // Start the hum sound for Afterlife pack
      }
      break;                     // Break out of the switch statement
    case PACK_MODE_SLIME_BLOWER: // Slime Blower
    case PACK_MODE_SLIME_TETHER: // Slime Tether
      sound_start_safely(
          25); // Start the hum sound for Slime Blower or Slime Tether
      break;   // Break out of the switch statement
    case PACK_MODE_STASIS_STREAM: // Stasis Stream
    case PACK_MODE_SHOCK_BLAST:   // Shock Blast
      sound_start_safely(
          34); // Start the hum sound for Stasis Stream or Shock Blast
      break;   // Break out of the switch statement
    case PACK_MODE_OVERLOAD_PULSE: // Overload Pulse
    default:                       // Meson Collider
      sound_start_safely(
          44); // Start the hum sound for Overload Pulse or Meson Collider
      break;   // Break out of the switch statement
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
      monster_timer = 240 * ((rand() % (pack_monster_timing.max_seconds -
                                        pack_monster_timing.min_seconds)) +
                             pack_monster_timing.min_seconds);
      do {
        temp_random_index = rand() % pack_monster_sound_pair_count;
      } while (temp_random_index == monster_sound_index);
      monster_sound_index = temp_random_index;
    } else if (monster_timer == 3) {
      sound_play_blocking(pack_monster_sound_pairs[monster_sound_index][0],
                          false, false);
      response_timer = pack_monster_timing.response_seconds * 240;
      monster_timer = 2;
    } else if ((monster_timer == 2) && (response_timer == 0)) {
      monster_timer = 0;
    } else if (monster_timer == 1) {
      sound_play_blocking(pack_monster_sound_pairs[monster_sound_index][1],
                          false, false);
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
 * @param apply_cy_speed Apply cyclotron speed multiplier when true. When set,
 *                       the ADJ0 potentiometer is ignored and a default
 *                       midpoint speed is used instead.
 * @return Computed cycle duration in milliseconds.
 */
uint16_t adj_to_ms_cycle(uint8_t adj_select, bool heat_effect,
                         bool apply_cy_speed) {
  uint32_t temp_calc = 0;
  if (apply_cy_speed) {
    // Use midpoint value so ADJ0 does not influence cyclotron speed
    temp_calc = pack_adj_min_ms + ((pack_adj_max_ms - pack_adj_min_ms) >> 1);
    temp_calc = (temp_calc * cy_speed_multiplier) >> 16;
  } else {
    temp_calc = pack_adj_min_ms + (((pack_adj_max_ms - pack_adj_min_ms) *
                                    (4095 - adj_pot[adj_select & 1])) >>
                                   12);
    if (heat_effect) {
      uint32_t divisor =
          pack_heat_settings[config_pack_type()].start_autovent >> 7;
      uint32_t heat_factor =
          (divisor > 0) ? ((temperature * 3) / (divisor * 2)) : 0;
      temp_calc = (temp_calc * (256 - heat_factor)) >> 8;
    }
  }

  temp_calc = (temp_calc >= pack_adj_max_ms) ? pack_adj_max_ms : temp_calc;
  temp_calc =
      (temp_calc <= pack_adj_min_ms >> 2) ? pack_adj_min_ms >> 2 : temp_calc;
  return temp_calc;
}

static void update_animation_speed(AnimationController &controller,
                                   uint16_t speed, uint16_t &last_speed) {
  if (speed == last_speed) {
    return;
  }
  Animation *anim = controller.getCurrentAnimation();
  if (anim) {
    anim->setSpeed(speed, 0);
  }
  last_speed = speed;
}

/**
 * @brief Update LED pattern speeds based on ADJ settings and pack heat.
 */
void adj_monitor(void) {
  bool heating_effect =
      config_dip_sw & DIP_HEAT_MASK; // is heat effect enabled?
  read_adj_potentiometers(true);     // read the ADC and average the samples

  static uint16_t last_pc_speed = 0;
  static uint16_t last_cy_speed = 0;

  uint16_t pc_speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, heating_effect, false);
  update_animation_speed(g_powercell_controller, pc_speed, last_pc_speed);

  uint16_t cy_speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, heating_effect, true);
  update_animation_speed(g_cyclotron_controller, cy_speed, last_cy_speed);
}

/**
 * @brief Perform a major mode change with coordinated sounds and lights.
 *
 * @param cyclotron_pattern_base Base cyclotron pattern index.
 * @param first_sound Sound to play during drain.
 * @param second_sound Optional sound to play with fade-in.
 */
void mode_change_major(uint8_t cyclotron_pattern_base, uint8_t first_sound,
                       uint8_t second_sound) {
  const bool afterlife_std = (config_pack_type() == PACK_TYPE_AFTERLIFE);
  const bool afterlife_tvg = (config_pack_type() == PACK_TYPE_AFTER_TVG);
  const bool afterlife_variant = afterlife_std || afterlife_tvg;

  sound_start_safely(first_sound);

  AnimationConfig pc_drain_config;
  pc_drain_config.leds = g_powercell_leds;
  pc_drain_config.num_leds = NUM_LEDS_POWERCELL;
  pc_drain_config.color =
      CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
  pc_drain_config.speed = 300;
  g_powercell_controller.play(std::make_unique<DrainAnimation>(),
                              pc_drain_config);

  if (!afterlife_variant) {
    AnimationConfig cy_fade_out_config;
    cy_fade_out_config.leds = g_cyclotron_leds;
    cy_fade_out_config.num_leds = g_cyclotron_led_count;
    cy_fade_out_config.color =
        CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_fade_out_config.speed = 300;
    g_cyclotron_controller.play(std::make_unique<FadeAnimation>(true),
                                cy_fade_out_config);
  }

  absolute_time_t start = get_absolute_time();
  while (g_powercell_controller.isRunning() ||
         (!afterlife_variant && g_cyclotron_controller.isRunning()) ||
         sound_is_playing()) {
    if (absolute_time_diff_us(start, get_absolute_time()) >
        MODE_CHANGE_TIMEOUT_MS * 1000) {
      break;
    }
    sleep_ms(20);
  }

  update_pack_colors();

  AnimationConfig pc_normal_config;
  pc_normal_config.leds = g_powercell_leds;
  pc_normal_config.num_leds = NUM_LEDS_POWERCELL;
  pc_normal_config.color =
      CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
  pc_normal_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
  g_powercell_controller.play(std::make_unique<ScrollAnimation>(),
                              pc_normal_config);

  start = get_absolute_time();
  while (sound_is_playing()) {
    if (absolute_time_diff_us(start, get_absolute_time()) >
        MODE_CHANGE_TIMEOUT_MS * 1000) {
      break;
    }
    sleep_ms(20);
  }

  if (second_sound != 0) {
    sound_start_safely(second_sound);
    if (!afterlife_variant) {
      AnimationConfig cy_fade_in_config;
      cy_fade_in_config.leds = g_cyclotron_leds;
      cy_fade_in_config.num_leds = g_cyclotron_led_count;
      cy_fade_in_config.color =
          CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
      cy_fade_in_config.speed = 1000;
      g_cyclotron_controller.play(std::make_unique<FadeAnimation>(false),
                                  cy_fade_in_config);

      start = get_absolute_time();
      while (g_cyclotron_controller.isRunning() || sound_is_playing()) {
        if (absolute_time_diff_us(start, get_absolute_time()) >
            MODE_CHANGE_TIMEOUT_MS * 1000) {
          break;
        }
        sleep_ms(20);
      }
    }
  }

  if (!afterlife_variant) {
    AnimationConfig cy_base_config;
    cy_base_config.leds = g_cyclotron_leds;
    cy_base_config.num_leds = g_cyclotron_led_count;
    cy_base_config.color =
        CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_base_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true);
    cy_base_config.clockwise = (config_cyclotron_dir() == 0);
    if (config_pack_type() == PACK_TYPE_FADE_RED ||
        config_pack_type() == PACK_TYPE_TVG_FADE) {
      cy_base_config.fade_amount = 4;
      cy_base_config.steps = 64;
      if (pack_state_get_mode() == PACK_MODE_SLIME_BLOWER ||
          pack_state_get_mode() == PACK_MODE_SLIME_TETHER) {
        g_cyclotron_controller.play(std::make_unique<SlimeAnimation>(),
                                    cy_base_config);
      } else {
        g_cyclotron_controller.play(std::make_unique<RotateFadeAnimation>(),
                                    cy_base_config);
      }
    } else {
      g_cyclotron_controller.play(std::make_unique<RotateAnimation>(),
                                  cy_base_config);
    }
  }
}

/**
 * @brief Monitor fire taps to cycle through available pack modes for TVG.
 */
void mode_monitor(void) {
  if (song_is_playing()) {
    return;
  }
  if (!fire_tap()) {
    return;
  }
  if ((config_pack_type() == PACK_TYPE_TVG_FADE) ||
      (config_pack_type() == PACK_TYPE_AFTER_TVG)) {
    PackMode prev = pack_state_get_mode();
    PackMode next = (PackMode)((int)prev + 1);
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
    if (config_pack_type() == PACK_TYPE_AFTER_TVG) {
      g_cyclotron_controller.enqueue(std::make_unique<ChangeColorAction>(
          CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b), 1000));
    }
  }
  clear_fire_tap();
}

/**
 * @brief Run a full vent sequence with sound and lighting effects.
 */
void full_vent(void) {
  cool_the_pack();
  sound_start_safely(55);
  const PackType pack_type = config_pack_type();
  const bool is_afterlife_pack =
      (pack_type == PACK_TYPE_AFTERLIFE) || (pack_type == PACK_TYPE_AFTER_TVG);
  AnimationConfig fr_config;
  fr_config.leds = g_future_leds;
  fr_config.num_leds = NUM_LEDS_FUTURE;
  fr_config.color = CRGB(future_color.r, future_color.g, future_color.b);

  if (is_afterlife_pack) {
    fr_config.speed = 600;
    fr_config.clockwise = false;
    g_future_controller.play(std::make_unique<ShiftRotateAnimation>(),
                             fr_config);
  } else {
    fr_config.speed = 150;
    g_future_controller.play(std::make_unique<StrobeAnimation>(), fr_config);
  }

  AnimationConfig pc_drain_config;
  pc_drain_config.leds = g_powercell_leds;
  pc_drain_config.num_leds = NUM_LEDS_POWERCELL;
  pc_drain_config.color =
      CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
  pc_drain_config.speed = 3600;
  g_powercell_controller.play(std::make_unique<DrainAnimation>(),
                              pc_drain_config);

  if (!is_afterlife_pack) {
    AnimationConfig cy_config;
    cy_config.leds = g_cyclotron_leds;
    cy_config.num_leds = g_cyclotron_led_count;
    cy_config.color =
        CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_config.speed = 3600;
    g_cyclotron_controller.play(std::make_unique<FadeAnimation>(true),
                                cy_config);
  }
  do {
    vent_light_on(true);
    sleep_ms(50);
    vent_light_on(false);
    sleep_ms(120);
  } while (vent_sw() || sound_is_playing());

  // Stop all animations that were started for the vent sequence.
  g_future_controller.stop();
  fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
  show_leds();
  g_powercell_controller.stop();
  if (!is_afterlife_pack) {
    g_cyclotron_controller.stop();
  }

  // Restore the idle animations
  AnimationConfig pc_normal_config;
  pc_normal_config.leds = g_powercell_leds;
  pc_normal_config.num_leds = NUM_LEDS_POWERCELL;
  pc_normal_config.color =
      CRGB(powercell_color.r, powercell_color.g, powercell_color.b);
  pc_normal_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false);
  g_powercell_controller.play(std::make_unique<ScrollAnimation>(),
                              pc_normal_config);

  if (is_afterlife_pack) {
    AnimationConfig cy_config;
    cy_config.speed = 1000;
    cy_config.color =
        CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_config.leds = g_cyclotron_leds;
    cy_config.num_leds = g_cyclotron_led_count;
    g_cyclotron_controller.play(std::make_unique<CylonAnimation>(), cy_config);
  } else {
    AnimationConfig cy_config;
    cy_config.leds = g_cyclotron_leds;
    cy_config.num_leds = g_cyclotron_led_count;
    cy_config.color =
        CRGB(cyclotron_color.r, cyclotron_color.g, cyclotron_color.b);
    cy_config.speed = adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true);
    cy_config.clockwise = (config_cyclotron_dir() == 0);
    g_cyclotron_controller.play(std::make_unique<RotateAnimation>(), cy_config);
  }
}

/**
 * @brief Monitor the vent switch and trigger vent sequences or quotes.
 */
void vent_monitor(void) {
  static uint8_t slime_quote_counter = 0;
  if (vent_sw() && pu_sw()) {
    if ((pack_state_get_mode() == PACK_MODE_SLIME_BLOWER) ||
        (pack_state_get_mode() == PACK_MODE_SLIME_TETHER)) {
      sound_play_blocking(150 + slime_quote_counter, false, false);
      slime_quote_counter = (slime_quote_counter + 1) % pack_slime_quote_count;
      do {
        sleep_ms(10);
      } while (vent_sw());
    } else {
      full_vent();
    }
  }
}

/**
 * @brief Adjust cyclotron LED ring size based on ADJ1 setting.
 * @details Reads the ADJ1 potentiometer and maps its value to one of the four
 *          allowed `N` values: {4, 24, 32, 40}. Changes are applied immediately
 *          and the remainder of the cyclotron LEDs are cleared so that reducing
 *          `N` never leaves stray pixels lit. When the value changes, it
 *          optionally triggers a brief rainbow animation on the cyclotron LEDs
 *          as feedback when the pack is off. The animation is handled by a
 *          temporary pack state so it does not interfere with normal
 *          cyclotron control.
 */
void ring_monitor(void) {
  if (party_mode_is_active())
    return;

  static const uint16_t HYSTERESIS = 0x80; // deadband around thresholds
  static uint8_t last_num_pixels = 0;

  read_adj_potentiometers(true);
  uint16_t raw = adj_pot[1];

  // On first run, map the raw value without hysteresis.
  if (last_num_pixels == 0) {
    if (raw < 0x180) {
      last_num_pixels = 4;
    } else if (raw < 0x800) {
      last_num_pixels = 24;
    } else if (raw < 0xE80) {
      last_num_pixels = 32;
    } else {
      last_num_pixels = 40;
    }
    g_cyclotron_led_count = last_num_pixels;
  }

  uint8_t current_num_pixels = last_num_pixels;

  switch (last_num_pixels) {
  case 4:
    if (raw > 0x180 + HYSTERESIS)
      current_num_pixels = 24;
    break;
  case 24:
    if (raw < 0x180 - HYSTERESIS)
      current_num_pixels = 4;
    else if (raw > 0x800 + HYSTERESIS)
      current_num_pixels = 32;
    break;
  case 32:
    if (raw < 0x800 - HYSTERESIS)
      current_num_pixels = 24;
    else if (raw > 0xE80 + HYSTERESIS)
      current_num_pixels = 40;
    break;
  case 40:
    if (raw < 0xE80 - HYSTERESIS)
      current_num_pixels = 32;
    break;
  default:
    break;
  }

  if (current_num_pixels > NUM_LEDS_CYCLOTRON) {
    current_num_pixels = NUM_LEDS_CYCLOTRON;
  }

  if (current_num_pixels != last_num_pixels) {
    last_num_pixels = current_num_pixels;
    g_cyclotron_led_count = current_num_pixels;

    // Trigger feedback animation when the pack is off or already showing
    // feedback. The animation itself runs in `PS_FEEDBACK` and blanks any
    // LEDs above the active count.
    if (pack_state_get_state() == PS_OFF || pack_state_get_state() == PS_FEEDBACK) {
      feedback_request();
    }
  }
}
