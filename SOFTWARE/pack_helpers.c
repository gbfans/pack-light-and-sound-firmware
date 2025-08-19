/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "pack_helpers.h"
#include "monitors.h"
#include "addressable_LED_support.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "klystron_IO_support.h"
#include "sound_module.h"
#include "pico/stdlib.h"

void pack_run_sequence(const PackSequence *seq) {
    sound_start_safely(seq->sound);
    pc_pattern_config(seq->pc_pattern, seq->pc_ms, PC_CYCLE_INFINITE);
    cy_pattern_config(seq->cy_pattern, seq->cy_ms, CY_CYCLE_INFINITE);
}

void pack_wait_sequence_end(void) {
    do {
        sleep_ms(20);
    } while (pc_pattern_running || cy_pattern_running || sound_is_playing());
    sleep_ms(10);
}

void pack_wait_user_or_complete(void) {
    do {
        sleep_ms(20);
        if (!pu_sw() && !pack_pu_sw() && !wand_standby_sw())
            break;
        if (fire_sw())
            break;
    } while (pc_pattern_running || cy_pattern_running || sound_is_playing());
}
