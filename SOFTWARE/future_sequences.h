/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef FUTURE_SEQUENCES_H
#define FUTURE_SEQUENCES_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint32_t future_color;

void fr_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num);
void fr_pattern_stop(bool clear_vars);
bool fr_pattern_is_running(void);
void fr_pattern_isr_ctrl(void);

#endif // FUTURE_SEQUENCES_H
