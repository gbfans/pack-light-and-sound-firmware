/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
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

/**
 * @brief Run startup sequence for the selected pack type.
 */
void pack_combo_startup(void) {
    switch (config_pack_type()) {
    case PACK_TYPE_SNAP_RED:
        // Normal pack startup uses steady patterns that never end. Kick off
        // the animations immediately and only wait for the sound to finish.
        pack_run_sequence(&(PackSequence){10, PC_PATTERN_NORMAL,
                                         adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false),
                                         CY_PATTERN_CLASSIC_ROTATE_RIGHT + config_cyclotron_dir(),
                                         adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true)});
        // Run one LED update cycle so the lights come up even if the timer has
        // not yet fired.
        pc_pattern_isr_ctrl();
        cy_pattern_isr_ctrl();
        sound_wait_til_end(true, true);
        break;
    case PACK_TYPE_FADE_RED:
        pack_run_sequence(&(PackSequence){10, PC_PATTERN_STARTUP, 4800,
                                         CY_PATTERN_FADE_IN, 4800});
        pack_wait_user_or_complete();
        pc_pattern_config(PC_PATTERN_NORMAL,
                          adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false),
                          PC_CYCLE_INFINITE);
        cy_pattern_config(CY_PATTERN_CLASSIC_ROTATE_FADE_RIGHT + config_cyclotron_dir(),
                          adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true),
                          CY_CYCLE_INFINITE);
        break;
    case PACK_TYPE_TVG_FADE:
        pack_run_sequence(&(PackSequence){58, PC_PATTERN_STARTUP, 4800,
                                         CY_PATTERN_FADE_IN, 4800});
        pack_wait_user_or_complete();
        pc_pattern_config(PC_PATTERN_NORMAL,
                          adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false),
                          PC_CYCLE_INFINITE);
        if (pack_state_get_mode() == PACK_MODE_SLIME_BLOWER ||
            pack_state_get_mode() == PACK_MODE_SLIME_TETHER) {
            cy_pattern_config(CY_PATTERN_SLIME_FADE_RIGHT + config_cyclotron_dir(),
                              adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true),
                              CY_CYCLE_INFINITE);
        } else {
            cy_pattern_config(CY_PATTERN_CLASSIC_ROTATE_FADE_RIGHT + config_cyclotron_dir(),
                              adj_to_ms_cycle(PC_SPEED_DEFAULT, false, true),
                              CY_CYCLE_INFINITE);
        }
        break;
    default:
        // The Afterlife startup sequence is a two-stage process.
        // First, run the one-shot power cell fill-up animation.
        pack_run_sequence(&(PackSequence){121, PC_PATTERN_STARTUP, 4800,
                                         CY_PATTERN_RING_CW, 1000});
        // Wait for the fill-up animation to complete, allowing for user interruption.
        do {
            sleep_ms(20); // wait for 20ms
            if (!pu_sw() && !pack_pu_sw() && !wand_standby_sw()) break;
            if (fire_sw()) break;
        } while (pc_pattern_running); // wait for the powercell pattern to finish
        // Now that the fill-up is complete, switch to the normal, cycling power cell animation.
        pc_pattern_config(PC_PATTERN_NORMAL,
                          adj_to_ms_cycle(PC_SPEED_DEFAULT, false, false),
                          PC_CYCLE_INFINITE);
        // Finally, wait for the main startup sound to finish, which is the longest part of the sequence.
        sound_wait_til_end(true,true);
        break;
    }
}

/**
 * @brief Play a brief powerup sound based on pack type.
 *
 * @param afterlife_higher Use alternate sound when true for afterlife packs.
 */
void pack_short_powerup_sound(bool afterlife_higher) {
    PackType type = config_pack_type();
    uint8_t sound = pack_short_powerup_sounds[type];
    if (type >= PACK_TYPE_AFTERLIFE && afterlife_higher)
        sound = 125;
    sound_play_blocking(sound, true, true);
}

/**
 * @brief Run powerdown LED and sound sequence for the current pack.
 */
void pack_combo_powerdown(void) {
    pack_run_sequence(&pack_powerdown_sequences[config_pack_type()]);
    pack_wait_sequence_end();
}

