/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef CYCLOTRON_SEQUENCES_H
#define CYCLOTRON_SEQUENCES_H

#include <stdbool.h>
#include <stdint.h>

// Cyclotron pattern identifiers
#define CY_PATTERN_RING_CW    0
#define CY_PATTERN_RING_CCW   1
#define CY_PATTERN_RING_FADE_OUT 2
#define CY_PATTERN_CLASSIC_ROTATE_RIGHT 3
#define CY_PATTERN_CLASSIC_ROTATE_LEFT  4
#define CY_PATTERN_CLASSIC_ROTATE_FADE_RIGHT 5
#define CY_PATTERN_CLASSIC_ROTATE_FADE_LEFT  6
#define CY_PATTERN_SLIME_FADE_RIGHT  7
#define CY_PATTERN_SLIME_FADE_LEFT   8
#define CY_PATTERN_VENT_FADE  9
#define CY_PATTERN_FADE_OUT   10
#define CY_PATTERN_FADE_IN    11
#define CY_PATTERN_DISPLAY_COUNT 12

// Cyclotron animation defaults
#define CY_CYCLE_INFINITE 0

extern volatile uint8_t cyc_classic_index;
extern volatile uint32_t cyclotron_after_set[12][3];
extern volatile uint8_t cy_color_set;

extern volatile uint8_t cy_pattern_num;
extern volatile bool cy_pattern_running;

void cy_pattern_speed_update(uint16_t ms_cycle);
void cy_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num);
void cy_pattern_stop(bool clear_vars);
bool cy_pattern_is_running(void);
void cy_pattern_isr_ctrl(void);
void cy_afterlife_init(void);

#endif // CYCLOTRON_SEQUENCES_H
