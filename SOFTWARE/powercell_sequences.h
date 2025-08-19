/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef POWERCELL_SEQUENCES_H
#define POWERCELL_SEQUENCES_H

#include <stdbool.h>
#include <stdint.h>

// Powercell pattern identifiers
#define PC_PATTERN_NORMAL        0
#define PC_PATTERN_FILL          1
#define PC_PATTERN_DRAIN         2
#define PC_PATTERN_SHUTDOWN      3
#define PC_PATTERN_OVERHEAT      4
#define PC_PATTERN_STARTUP       5

// Powercell animation timing and cycle defaults
#define PC_SPEED_DEFAULT  0
#define PC_CYCLE_INFINITE 0

extern volatile uint8_t powercell_seq_num;
extern volatile uint32_t powercell_color;

extern volatile uint8_t pc_pattern_num;
extern volatile bool pc_pattern_running;

void pc_pattern_speed_update(uint16_t ms_cycle);
void pc_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num);
void pc_pattern_stop(bool clear_vars);
bool pc_pattern_is_running(void);
void pc_pattern_isr_ctrl(void);

#endif // POWERCELL_SEQUENCES_H
