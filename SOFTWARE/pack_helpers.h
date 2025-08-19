/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef PACK_HELPERS_H
#define PACK_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief LED and sound sequence executed during power events.
 */
typedef struct {
    uint8_t sound;      /**< Sound index to play. */
    uint8_t pc_pattern; /**< Powercell pattern identifier. */
    uint16_t pc_ms;     /**< Duration for powercell pattern in milliseconds. */
    uint8_t cy_pattern; /**< Cyclotron pattern identifier. */
    uint16_t cy_ms;     /**< Duration for cyclotron pattern in milliseconds. */
} PackSequence;

/** Run a pack sequence and play associated sound. */
void pack_run_sequence(const PackSequence *seq);

/** Wait for the current sequence to complete. */
void pack_wait_sequence_end(void);

/** Wait for user interaction or sequence completion. */
void pack_wait_user_or_complete(void);

#endif // PACK_HELPERS_H
