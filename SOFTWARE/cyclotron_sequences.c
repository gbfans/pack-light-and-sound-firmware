/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: cyclotron_sequences.c
 * Version 1.0.0
 * Date: 2025-06-06
 *
 * This file is part of the GhostLab42 Support for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 *
 * The cyclotron sequences are used in the ISR to somewhat autonomously provide sequences on the addressable
 * LED string. No "sleep" commands should be used since these routines should be called inside the ISR and
 * need to execute quickly.
 */

#include "cyclotron_sequences.h"
#include "addressable_LED_support.h"
#include "pico/stdlib.h"
#include "colors.h"

#define cyclotron_strip (*get_cyclotron_strip())
 
 // aid for getting the classic cyclotron positions
// const uint8_t cyclotron_classic_pos [4] = {CYCLOTRON_CLASSIC_POS_1, CYCLOTRON_CLASSIC_POS_2, CYCLOTRON_CLASSIC_POS_3, CYCLOTRON_CLASSIC_POS_4}; // Positions for the classic cyclotron
volatile uint8_t cyc_classic_index = 3; // start out with the 40 position set of numbers

//Afterlife with 3 different tails, depending on speed
const uint32_t cyclotron_after_set_size = 3; // Start with 3 pixels?
volatile uint32_t cyclotron_after_set[12][3];
// dgk update for fade correction so we do not have to be just max colors ????

// set up for 5 pixels max
volatile uint32_t cyclotron_color_set_size = 1; // Start with 5 pixels?
volatile uint32_t cyclotron_color_set[4][5];

// global-ish variables for the cyclotron sequence
volatile uint8_t cyclotron_seq_num = 0; // sequence number for the cyclotron
volatile uint8_t cy_color_set = 0; // set main color for the cyclotron classic modes: 0 Proton, 1 Slime, 2 Stasis, 3 Meson

// fade rate constants for the afterlife rotational mode
//const uint16_t delta_1 = 128; // slow starting speed
const uint16_t delta_1 = 128; // slow starting speed
const uint16_t delta_2 = 1024; // change to longer
const uint16_t delta_3 = 2048; // max speed and longest
const uint16_t delta_4 = 4048; // max speed and longest

/**
 * @brief Apply fade correction to the afterlife color set.
 */
void cy_afterlife_init(void) {
    uint32_t colors[4] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE};
    for (int c = 0; c < 4; ++c) {
        uint32_t base = colors[c];
        cyclotron_after_set[c*3 + 0][0] = 0;
        cyclotron_after_set[c*3 + 0][1] = 0;
        cyclotron_after_set[c*3 + 0][2] = base;
        cyclotron_after_set[c*3 + 1][0] = 0;
        cyclotron_after_set[c*3 + 1][1] = base & 0x1F1F1F1F;
        cyclotron_after_set[c*3 + 1][2] = base;
        cyclotron_after_set[c*3 + 2][0] = base & 0x1F1F1F1F;
        cyclotron_after_set[c*3 + 2][1] = base & 0x3F3F3F3F;
        cyclotron_after_set[c*3 + 2][2] = base;

        for (int i = 0; i < 5; i++) {
            cyclotron_color_set[c][i] = (i < 3) ? base : 0;
        }
    }

    for (int i = 0; i <= 11; i++) {
        switch (i % 3) {
        case 0:
            cyclotron_after_set[i][1] = 0;
            cyclotron_after_set[i][0] = 0;
            break;
        case 1:
            cyclotron_after_set[i][1] = fade_correction(cyclotron_after_set[i][2], 210);
            cyclotron_after_set[i][0] = 0;
            break;
        default:
            cyclotron_after_set[i][1] = fade_correction(cyclotron_after_set[i][2], 163);
            cyclotron_after_set[i][0] = fade_correction(cyclotron_after_set[i][2], 210);
            break;
        }
    }
}


// afterlife with ring
void cy_rotate(bool seq_init) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t pos_accum = 0; // accumulator for the position to hold onto the fractional portion of the position, fractional in lower 12 bits, integer in upper 4 bits
    static uint16_t delta_accum = 0; // holds the delta value that should grow as the speed increases
    static uint8_t after_set = 0; // the index of the after set to use, 0-3
    if (seq_init) {
        // init local variables
        pos_accum = 0; // start at 0
        delta_accum = delta_1; // start with the slowest speed
        after_set = 0; // start with the first set
        // start centered on one of the cyclotron openings
//        cyclotron_seq_num = (cyclotron_classic_pos[0] >= 2) ? cyclotron_classic_pos[0]-2 : (cyclotron_classic_pos[0] == 1) ? 0 :  cyclotron_strip.num_pixels-1; // allow to wrap around
        cyclotron_seq_num = cyclotron_positions.classic[cyc_classic_index][0]; // start at the first cyclotron position
        // light up the pixel
        for(int i=0; i<cyclotron_after_set_size; i++) {
            cyclotron_strip.grbx[((cyclotron_seq_num)+ i) % cyclotron_strip.num_pixels] = cyclotron_after_set[after_set+(cy_color_set*3)][i]; // turn on a set of LEDs
        }
    } else {
        // calculate a new position
        pos_accum += delta_accum; // accumulate the position
        // calculate a new speed if not maxed out already, scaled by number of LEDs
        uint16_t scale = (cyclotron_strip.num_pixels * 256) / 40;
        uint16_t increment;
        if (delta_accum < delta_2) {
            increment = (1 * scale) >> 8;
            delta_accum += (increment > 0) ? increment : 1;
        } 
        else if (delta_accum == delta_2) {
            increment = (2 * scale) >> 8;
            delta_accum += (increment > 0) ? increment : 1;
            after_set = 1; // move to the next after set
        }
        else if (delta_accum < delta_3) {
            increment = (2 * scale) >> 8;
            delta_accum += (increment > 0) ? increment : 1;
        }
        else if (delta_accum == delta_3) {
            after_set = 2; // move to the next after set
            increment = (2 * scale) >> 8;
            delta_accum += (increment > 0) ? increment : 1;
        }
        else if (delta_accum < delta_4) {
            increment = (4 * scale) >> 8;
            delta_accum += (increment > 0) ? increment : 1;
        }
        if (pos_accum >= 1<<12) { // the position accumulator has an integer portion, we need to shift the LEDs
            uint16_t prev_seq = cyclotron_seq_num;
            // add the integer portion to the cyclotron_seq_num
            cyclotron_seq_num += (pos_accum >> 12); // add the integer portion of the position to the sequence number
            if (cyclotron_seq_num >= cyclotron_strip.num_pixels) { // if the sequence number is greater than or equal to the number of pixels, wrap around
                cyclotron_seq_num -= cyclotron_strip.num_pixels; // wrap around
            }
            // clear out the integer portion that was added to the position
            pos_accum &= 0x0FFF; // keep only the fractional portion of the position
            // clear the previous position
            for (int i=0; i<cyclotron_after_set_size; i++) {
                cyclotron_strip.grbx[(prev_seq + i) % cyclotron_strip.num_pixels] = 0; // clear the previous LEDs
            }
            // turn on the new position
            for(int i=0; i<cyclotron_after_set_size; i++) {
                cyclotron_strip.grbx[((cyclotron_seq_num)+ i) % cyclotron_strip.num_pixels] = cyclotron_after_set[after_set+(cy_color_set*3)][i]; // turn on a set of LEDs
            }
        }
    }
}

// afterlife with only 4 LEDs
void cy_rotate_afterx4(bool seq_init) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t pos_accum = 0; // accumulator for the position to hold onto the fractional portion of the position, fractional in lower 12 bits, integer in upper 4 bits
    static uint16_t delta_accum = 0; // holds the delta value that should grow as the speed increases
    if (seq_init) {
        // init local variables
        pos_accum = 0; // start at 0
        delta_accum = delta_1; // start with the slowest speed
        // start centered on one of the cyclotron openings
//        cyclotron_seq_num = (cyclotron_classic_pos[0] >= 2) ? cyclotron_classic_pos[0]-2 : (cyclotron_classic_pos[0] == 1) ? 0 :  cyclotron_strip.num_pixels-1; // allow to wrap around
        cyclotron_seq_num = cyclotron_positions.classic[cyc_classic_index][0]; // start at the first cyclotron position
        // light up the pixel
        cyclotron_strip.grbx[cyclotron_seq_num % cyclotron_strip.num_pixels] = cyclotron_after_set[cy_color_set*3][2]; // turn on a set of LEDs
    } else {
        // calculate a new position
        pos_accum += delta_accum; // accumulate the position
        // calculate a new speed if not maxed out already
        if (delta_accum < delta_3) {
            delta_accum += 1; // increase the delta position to speed up the rotation
        }
//        if (pos_accum >= 1<<12) { // the position accumulator has an integer portion, we need to shift the LEDs
        if (pos_accum >= 1<<14) { // the position accumulator has an integer portion, we need to shift the LEDs
            uint16_t prev_seq = cyclotron_seq_num;
            // add the integer portion to the cyclotron_seq_num
            cyclotron_seq_num += (pos_accum >> 14); // add the integer portion of the position to the sequence number
            if (cyclotron_seq_num >= cyclotron_strip.num_pixels) { // if the sequence number is greater than or equal to the number of pixels, wrap around
                cyclotron_seq_num -= cyclotron_strip.num_pixels; // wrap around
            }
            // clear out the integer portion that was added to the position
            pos_accum &= 0x0FFF; // keep only the fractional portion of the position
            // clear the previous position
            cyclotron_strip.grbx[prev_seq % cyclotron_strip.num_pixels] = 0;
            // turn on the new position
            cyclotron_strip.grbx[cyclotron_seq_num % cyclotron_strip.num_pixels] = cyclotron_after_set[cy_color_set*3][2]; // turn on a set of LEDs
        }
    }
}

// Afterlife with ring
bool cy_rotate_fade_out(bool seq_init, uint16_t fade_amount) {
    // This pattern assumes the rotating is at the max and just fades it down to nothing.  THe initial position will be wrong, but it is so fast it is not an issue.
    static uint16_t pos_accum = 0; // accumulator for the position to hold onto the fractional portion of the position, fractional in lower 12 bits, integer in upper 4 bits
    static uint16_t delta_accum = 0; // holds the delta value that should grow as the speed increases
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        // cyclotron_seq_num = 0;  No change to whatever the current position is
        // init local variables
        pos_accum = 0; // start at 0
//        delta_accum = delta_4; // start with the slowest speed
        delta_accum = delta_4; // start with the slowest speed
        fade_value = 0; // start with no fade
        // light up the pixel
        for(int i=0; i<cyclotron_after_set_size; i++) {
            cyclotron_strip.grbx[((cyclotron_seq_num)+ i) % cyclotron_strip.num_pixels] = cyclotron_after_set[(cy_color_set*3)][i]; // turn on a set of LEDs
        }
    } else {
        // calculate the next fade value
        fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        // calculate a new position
        pos_accum += delta_accum; // accumulate the position
        // calculate a new speed if not maxed out already
        if (delta_accum > delta_1) {
            delta_accum -= fade_amount>>4; // decrease the delta position to speed up the rotation scaled to the fade rate
        } 
        // check to see if the pixels need to be shifted.
        if (pos_accum >= 1<<12) { // the position accumulator has an integer portion, we need to shift the LEDs
            uint16_t prev_seq = cyclotron_seq_num;
            // add the integer portion to the cyclotron_seq_num
            cyclotron_seq_num += (pos_accum >> 12); // add the integer portion of the position to the sequence number
            if (cyclotron_seq_num >= cyclotron_strip.num_pixels) { // if the sequence number is greater than or equal to the number of pixels, wrap around
                cyclotron_seq_num -= cyclotron_strip.num_pixels; // wrap around
            }
            // clear out the integer portion that was added to the position
            pos_accum &= 0x0FFF; // keep only the fractional portion of the position
            // clear the previous position
            for (int i=0; i<cyclotron_after_set_size; i++) {
                cyclotron_strip.grbx[(prev_seq + i) % cyclotron_strip.num_pixels] = 0; // clear the previous LEDs
            }
            // turn on the new position
            for(int i=0; i<cyclotron_after_set_size; i++) {
                cyclotron_strip.grbx[((cyclotron_seq_num)+ i) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_after_set[(cy_color_set*3)][i], fade_value >> 8); // turn on a set of LEDs
            }
        }
    }
    if (fade_value >= (255 << 8) ) {
        for (int i = 0; i < cyclotron_after_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_seq_num + i) % cyclotron_strip.num_pixels] = 0;
        }
    }
    return (fade_value >= (255 << 8) ); // return true if the sequence is complete, i.e. the passed fade value is 255
}

// afterlife only 4 LEDs
bool cy_rotate_fade_out_afterx4(bool seq_init, uint16_t fade_amount) {
    // This pattern assumes the rotating is at the max and just fades it down to nothing.  THe initial position will be wrong, but it is so fast it is not an issue.
    static uint16_t pos_accum = 0; // accumulator for the position to hold onto the fractional portion of the position, fractional in lower 12 bits, integer in upper 4 bits
    static uint16_t delta_accum = 0; // holds the delta value that should grow as the speed increases
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        // cyclotron_seq_num = 0;  No change to whatever the current position is
        // init local variables
        pos_accum = 0; // start at 0
//        delta_accum = delta_4; // start with the slowest speed
        delta_accum = delta_3; // start with the slowest speed
        fade_value = 0; // start with no fade
        // light up the pixel
         cyclotron_strip.grbx[cyclotron_seq_num % cyclotron_strip.num_pixels] = cyclotron_after_set[cy_color_set*3][2]; // turn on a set of LEDs
    } else {
        // calculate the next fade value
        fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        // calculate a new position
        pos_accum += delta_accum; // accumulate the position
        // calculate a new speed if not maxed out already
        if (delta_accum > delta_1) {
//            delta_accum -= fade_amount>>4; // decrease the delta position to speed up the rotation scaled to the fade rate
            delta_accum -= fade_amount>>5; // decrease the delta position to speed up the rotation scaled to the fade rate
        } 
        // check to see if the pixels need to be shifted.
//        if (pos_accum >= 1<<12) { // the position accumulator has an integer portion, we need to shift the LEDs
        if (pos_accum >= 1<<14) { // the position accumulator has an integer portion, we need to shift the LEDs
            uint16_t prev_seq = cyclotron_seq_num;
            // add the integer portion to the cyclotron_seq_num
            cyclotron_seq_num += (pos_accum >> 14); // add the integer portion of the position to the sequence number
            if (cyclotron_seq_num >= cyclotron_strip.num_pixels) { // if the sequence number is greater than or equal to the number of pixels, wrap around
                cyclotron_seq_num -= cyclotron_strip.num_pixels; // wrap around
            }
            // clear out the integer portion that was added to the position
            pos_accum &= 0x0FFF; // keep only the fractional portion of the position
            // clear the previous position
            cyclotron_strip.grbx[prev_seq % cyclotron_strip.num_pixels] = 0;
            // turn on the new position
            cyclotron_strip.grbx[cyclotron_seq_num % cyclotron_strip.num_pixels] = fade_correction(cyclotron_after_set[cy_color_set*3][2], fade_value >> 8); // turn on a set of LEDs
        }
    }
    if (fade_value >= (255 << 8) ) {
        cyclotron_strip.grbx[cyclotron_seq_num % cyclotron_strip.num_pixels] = 0;
    }
    return (fade_value >= (255 << 8) ); // return true if the sequence is complete, i.e. the passed fade value is 255
}

const uint16_t cy_classic_rotate_cycles = 4; // one step per LED in the ring

void cy_classic_rotate(bool seq_init, bool clockwise) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    if (seq_init) {
        cyclotron_seq_num = 0; // reset the sequence number
        //set the current position to the first classic position
        for (int i=0; i<cyclotron_color_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
        }
    } else {
        // clear out the previous position
        for (int i=0; i<cyclotron_color_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = 0; // turn off the classic cyclotron LEDs
        }
        // calculate new position
        if (clockwise) {
            cyclotron_seq_num = (cyclotron_seq_num+1) % 4; // increment the sequence number
        } else {
            cyclotron_seq_num = (cyclotron_seq_num+3) % 4; // decrement the sequence number
        }   
        // turn on the new position
        for (int i=0; i<cyclotron_color_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
        }
    }
}


const uint16_t cy_classic_rotate_fade_cycles = 256; // one step, not for all four positions

void cy_classic_rotate_fade(bool seq_init, bool clockwise, uint16_t fade_amount, uint16_t steps_actual) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t sub_seq1 = 0; // local subsequence number for a more complex pattern
    static uint16_t sub_seq2 = 0; // local subsequence number for a more complex pattern
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    static uint8_t prev_position = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        cyclotron_seq_num = 0; // reset the sequence number
        // init local variables
        sub_seq1 = 0; // reset the subsequence number
        sub_seq2 = 0; // reset the subsequence number
        fade_value = 0; // set to the no fade for starters
        prev_position = 0; // reset the previous position number
        // light the first position before clearing others
        for (int i = 0; i < cyclotron_color_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i];
        }
        // clear the remaining classic positions
        for (int j = 1; j < 4; j++) {
            for (int i = 0; i < cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = 0;
            }
        }
    } else {
        sub_seq1++; // increment the subsequence number
        fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        if (sub_seq2 > 0) {
            // fade_out the previous position
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][prev_position]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], fade_value >> 8);
            }
        }
        if (sub_seq1 >= steps_actual) {
            fade_value = 0; // reset the fade value
            sub_seq1 = 0; // reset the subsequence number
            sub_seq2 = 1; // start fading the previous positions
            prev_position = cyclotron_seq_num; // save the previous position
            // calculate new position?
            if (clockwise) {
                cyclotron_seq_num = (cyclotron_seq_num+1) % 4; // increment the sequence number
            } else {
                cyclotron_seq_num = (cyclotron_seq_num+3) % 4; // decrement the sequence number
            }
            // turn on the new position
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
            }
        }
    }
}

void cy_classic_slime(bool seq_init, bool clockwise, uint16_t fade_amount, uint16_t steps_actual) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t sub_seq1 = 0; // local subsequence number for a more complex pattern
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    static uint8_t prev_position = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        cyclotron_seq_num = 0; // reset the sequence number
        // init local variables
        sub_seq1 = 0; // reset the subsequence number
        fade_value = 0; // set to the no fade for starters
        prev_position = 0; // reset the previous position number
        //Turn on all 4 of the classic cyclotron LEDs
        for (int j=0; j<4; j++) {
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
            }
        }
    } else {
        sub_seq1++; // increment the subsequence number
        fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        // fade_out the current position
        for (int i=0; i<cyclotron_color_set_size; i++) {
            cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][cyclotron_seq_num]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], fade_value >> 8);
        }
        if (sub_seq1 >= steps_actual) {
            fade_value = 0; // reset the fade value
            sub_seq1 = 0; // reset the subsequence number
            prev_position = cyclotron_seq_num; // save the previous position
            // calculate new position?
            if (clockwise) {
                cyclotron_seq_num = (cyclotron_seq_num+1) % 4; // increment the sequence number
            } else {
                cyclotron_seq_num = (cyclotron_seq_num+3) % 4; // decrement the sequence number
            }
            // turn on the previous position
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][prev_position]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
            }
        }
    }
}


const uint16_t cy_classic_fade_cycles = 256; // really only one cycle since no positional changes, but the fadeout needs to be done multiple times.

bool cy_classic_strobe(bool seq_init, uint16_t fade_amount, uint16_t steps_actual) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t sub_seq1 = 0; // local subsequence number for a more complex pattern
    static uint16_t sub_seq2 = 0; // local subsequence number for a more complex pattern
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        // init local variables
        sub_seq1 = 0; // reset the subsequence number
        sub_seq2 = 0; // reset the subsequence number
        fade_value = 0; // set to the no fade for starters
        // light up half of the cyclotron positions
        for(int j=0; j<2; j++) {
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j*2]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = cyclotron_color_set[cy_color_set][i]; // turn on the classic cyclotron LEDs
            }
        }
    } else {
        if (sub_seq1 >= steps_actual) {
            sub_seq1 = 0; // reset the subsequence number
            sub_seq2 = sub_seq2 ^ 0x01; // increment the subsequence number, but only two positions
        } else {
            sub_seq1++; // increment the subsequence number
        }
        // calculate the next fade value
        if (sub_seq2 == 0) {
            fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        } else {
            fade_value = (fade_value > fade_amount) ? fade_value - fade_amount : 0; // decrement the fade value, but needs to not go below 0
        }  
        // Fade all of the cyclotrons - half getting brighter and half getting dimmer 
        for(int j=0; j<2; j++) {
            for (int i=0; i<cyclotron_color_set_size; i++) {
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j*2]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], fade_value >> 8);
                cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][1+j*2]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], 255-(fade_value >> 8));
            }   
        }
    }
}

bool cy_classic_all_fade(bool seq_init, uint16_t fade_amount, bool fade_out) {
    // This pattern rotates the cyclotron LEDs by shifting them around in one direction or the other
    static uint16_t fade_value = 0; // scaled up by a factor of 256, so 0-65535
    if (seq_init) {
        // init local variables
        fade_value = 0; // set to the no fade for fade_out, or max fade for fade_in
    } else {
        // calculate the next fade value
        fade_value = ((fade_value + fade_amount) < 1<<16) ? fade_value + fade_amount : (1<<16)-1; // increment the fade value, but needs to max out at 2^16-1
        // apply the fade value to the cyclotron color set
        for(int j=0; j<4; j++) {
            for (int i=0; i<cyclotron_color_set_size; i++) {
                if (fade_out) {
                    // fade out the cyclotron color set
                    cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], fade_value >> 8);
                } else {
                    // fade in the cyclotron color set
                    cyclotron_strip.grbx[(cyclotron_positions.classic[cyc_classic_index][j]+i+cyclotron_strip.num_pixels-(cyclotron_color_set_size>>1)) % cyclotron_strip.num_pixels] = fade_correction(cyclotron_color_set[cy_color_set][i], 255-(fade_value >> 8));
                }
            }
        }
    }
    return (fade_value >= (255 << 8) ); // return true if the sequence is complete, i.e. the passed fade value is 255
}


// cyclotron pattern control variables
volatile uint32_t cy_pattern_ms_sleep = 0;     // number of ms between running the pattern, scaled by 256
volatile uint16_t cy_pattern_fade_delta = 0;   // fade delta for the cyclotron pattern
volatile uint16_t cy_pattern_steps_actual = 0; // actual number of steps needed for the fade out pattern to complete, scaled by 256
volatile uint32_t cy_pattern_ms_count = 0;     // current number of ms since last pattern was run, scaled by 256
volatile uint8_t  cy_pattern_num = 0;          // current pattern number
volatile bool     cy_pattern_init = false;     // pattern initialization flag
volatile uint16_t cy_pattern_cycle_num = 0;    // number of cycles for the pattern, 0 for infinite
volatile bool     cy_pattern_running = 0;      // number of cycles for the pattern, 0 for infinite

//volatile uint32_t debug = 0; // counter
//volatile uint32_t debug2 = 0; // counter


/****************************
 * int main() use functions *
 * **************************/  

 // update the values that control the speed - can be dynamically updated for heating affects
void cy_pattern_speed_update(uint16_t ms_cycle) {
    uint32_t temp_ms_sleep = 0; // useful for only updating the pc_pattern_ms_sleep only one time
    switch (cy_pattern_num) { // based on the current pattern number
        case 0: // Ring Rotate CW
        case 1: // Ring Rotate CCW
        case 2: // Ring fade out
            temp_ms_sleep = 4*256; // Just go the maximum rate of 4ms
            cy_pattern_fade_delta = (256 * cy_classic_fade_cycles) / (ms_cycle / 4); // calculate the fade change per step, scaled up by a factor of 256
            break;
        case 3: // Classic 4 Rotate Right
        case 4: // Classic 4 Rotate Left
            temp_ms_sleep = 4 * (256 * ms_cycle) / cy_classic_rotate_cycles; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            temp_ms_sleep = (temp_ms_sleep > 4*256) ? temp_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            break;
        case 5: // Classic 4 Rotate & fade Right
        case 6: // Classic 4 Rotate & fade Left
        case 7: // Classic Slime fade Right
        case 8: // Classic Slime fade Left
            // high precision step calculation
            temp_ms_sleep = 4 * (256 * ms_cycle /4) / cy_classic_rotate_fade_cycles; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            temp_ms_sleep = (temp_ms_sleep > 4*256) ? temp_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            // high precision fade fraction calculation
            cy_pattern_steps_actual = (256 * ms_cycle) / temp_ms_sleep; // determine the actual number of steps needed for the fade out pattern to complete
            cy_pattern_fade_delta = (256 * cy_classic_rotate_fade_cycles) / cy_pattern_steps_actual; // calculate the fade change per step, scaled up by a factor of 256
            break;
        case 9: // Classic 4 Strobe (Overheat)
        case 10: // Classic 4 Fade Out (PowerDown)
        case 11: // Classic 4 Fade In (PowerUp)
            // high precision step calculation
            temp_ms_sleep = (256 * ms_cycle) / cy_classic_fade_cycles; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            temp_ms_sleep = (temp_ms_sleep > 4*256) ? temp_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            // high precision fade fraction calculation
            cy_pattern_steps_actual = (256 * ms_cycle) / temp_ms_sleep; // determine the actual number of steps needed for the fade out pattern to complete
            cy_pattern_fade_delta = (256 * cy_classic_fade_cycles) / cy_pattern_steps_actual; // calculate the fade change per step, scaled up by a factor of 256
            break;
        case 12: // Display LED count
            temp_ms_sleep = 256 * ms_cycle; // run pattern for specified ms_cycle once
            break;
        default:
            break; // do nothing if the pattern number is not valid
    }
    // update the variable only 1 time
    cy_pattern_ms_sleep = temp_ms_sleep;

} // end of cy_pattern_speed_update

// This function will set the pattern number, ms sleep time, and cycle number for the cyclotron pattern control
// It will also start the pattern running - at least indirectly by setting the pattern running flag
// This function is intended to be called from the main program to configure the cyclotron pattern control
void cy_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num) {
    // to make sure we do not update the variables in the middle of a pattern update, 
    // stop the running of a currently running pattern
    if (cy_pattern_running) {
        cy_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // these are just pass throughs
    cy_pattern_num = pattern_num; // set the pattern number
    cy_pattern_cycle_num = cycle_num; // set the cycle number
   
    // calculate several values to dynamically control speed
    cy_pattern_speed_update(ms_cycle); // update cy_pattern_ms_sleep, 

 /*   switch (pattern_num) {
        case 0: // Ring Rotate CW
        case 1: // Ring Rotate CCW
        case 2: // Ring fade out
            cy_pattern_ms_sleep = 4*256; // Just go the maximum rate of 4ms
            cy_pattern_fade_delta = (256 * cy_classic_fade_cycles) / (ms_cycle / 4); // calculate the fade change per step, scaled up by a factor of 256
            break;
        case 3: // Classic 4 Rotate Right
        case 4: // Classic 4 Rotate Left
            cy_pattern_ms_sleep = (256 * ms_cycle) / 4; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            cy_pattern_ms_sleep = (cy_pattern_ms_sleep > 4*256) ? cy_pattern_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            break;
        case 5: // Classic 4 Rotate & fade Right
        case 6: // Classic 4 Rotate & fade Left
        case 7: // Classic Slime fade Right
        case 8: // Classic Slime fade Left
            // high precision step calculation
            cy_pattern_ms_sleep = (256 * ms_cycle /4) / cy_classic_rotate_fade_cycles; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            cy_pattern_ms_sleep = (cy_pattern_ms_sleep > 4*256) ? cy_pattern_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            // high precision fade fraction calculation
            cy_pattern_steps_actual = (256 * ms_cycle /4) / cy_pattern_ms_sleep; // determine the actual number of steps needed for the fade out pattern to complete
            cy_pattern_fade_delta = (256 * cy_classic_rotate_fade_cycles) / cy_pattern_steps_actual; // calculate the fade change per step, scaled up by a factor of 256
            break;
        case 9: // Classic 4 Strobe (Overheat)
        case 10: // Classic 4 Fade Out (PowerDown)
        case 11: // Classic 4 Fade In (PowerUp)
            // high precision step calculation
            cy_pattern_ms_sleep = (256 * ms_cycle) / cy_classic_fade_cycles; // calculate the step size for the fade out pattern, scaled up by a factor of 256
            cy_pattern_ms_sleep = (cy_pattern_ms_sleep > 4*256) ? cy_pattern_ms_sleep : 4 *256; // make sure we keep a minimum value that equates to the 4ms ISR rate
            // high precision fade fraction calculation
            cy_pattern_steps_actual = (256 * ms_cycle) / cy_pattern_ms_sleep; // determine the actual number of steps needed for the fade out pattern to complete
            cy_pattern_fade_delta = (256 * cy_classic_fade_cycles) / cy_pattern_steps_actual; // calculate the fade change per step, scaled up by a factor of 256
            break;
        default:
            break; // do nothing if the pattern number is not valid
    }
*/

    // this will start a pattern running
    cy_pattern_init = true; // set the pattern initialization flag
    cy_pattern_ms_count = 0; // reset the ms counter
    cy_pattern_running = true; // set the pattern running flag to true, needs to be the last thing to update
}

// This function will stop the pattern running and optionally clear the cyclotron variables    
void cy_pattern_stop(bool clear_vars) {
    // stop the running of any currently running pattern
    if (cy_pattern_running) {
        cy_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // Optionally force all of the LEDs in the string off
    if (clear_vars) {
        clear_strip(&cyclotron_strip); // clear the cyclotron vars
        sleep_ms(5); // wait long enough to finish the transfer and have the LEDs see an end of transmission condition
        start_xfer_cyclotron(); // start a transfer for cyclotron
    }
}

// This function will return the status of the pattern running flag
bool cy_pattern_is_running() {
    return cy_pattern_running; // return the pattern running flag
}

/*****************
 * ISR Functions *
 *****************/

// This function will start a pattern running 
// Intended use is only for the cy_pattern_isr_ctrl() function
void cy_pattern_run(uint8_t pattern_num, bool seq_init) {
    // local variables for the pattern control
    bool done = false; // pattern done flag, defaults to not done
    // check the pattern number and run the appropriate sequence
    //so far, all sequences run forever, so must be stopped by a cy_pattern_stop call
    switch (pattern_num) {
        case 0: // Ring Rotate CW
            if (cyclotron_strip.num_pixels == 4) {
                cy_rotate_afterx4(seq_init); // Rotate a single set of pixels around the cyclotron string in CW direction
            } else {
                cy_rotate(seq_init); // Rotate a single set of pixels around the cyclotron string in CW direction
            }
            break;
        case 1: // Ring Rotate CCW ??
            break;
        case 2: // Ring fade out
            if (cyclotron_strip.num_pixels == 4) {
                done = cy_rotate_fade_out_afterx4(seq_init, cy_pattern_fade_delta); // Rotate a single "off" pixel around the cyclotron string in CCW direction
            } else {
                done = cy_rotate_fade_out(seq_init, cy_pattern_fade_delta); // Rotate a single "off" pixel around the cyclotron string in CCW direction
            }
            break;
        case 3: // Classic 4 Rotate Right
            cy_classic_rotate(seq_init, true); // Rotate a single block of pixels around the cyclotron string in 4 discrete positions in a CCW direction
            break;
        case 4: // Classic 4 Rotate Left
            cy_classic_rotate(seq_init, false); // Rotate a single block of pixels around the cyclotron string in 4 discrete locations in a CCW direction
            break;
        case 5: // Classic 4 Rotate Right
            cy_classic_rotate_fade(seq_init, true, cy_pattern_fade_delta*2, cy_pattern_steps_actual); // Rotate a single block of pixels around the cyclotron string in 4 discrete positions in a CCW direction
            break;
        case 6: // Classic 4 Rotate Left
            cy_classic_rotate_fade(seq_init, false, cy_pattern_fade_delta*2, cy_pattern_steps_actual); // Rotate a single block of pixels around the cyclotron string in 4 discrete locations in a CCW direction
            break;
        case 7: // Classic 4 Slime Right
            cy_classic_slime(seq_init, true, cy_pattern_fade_delta*2, cy_pattern_steps_actual); // Rotate a single block of pixels around the cyclotron string in 4 discrete positions in a CCW direction
            break;
        case 8: // Classic 4 Slime Left
            cy_classic_slime(seq_init, false, cy_pattern_fade_delta*2, cy_pattern_steps_actual); // Rotate a single block of pixels around the cyclotron string in 4 discrete locations in a CCW direction
            break;
        case 9: // Classic 4 Strobe (Overheat)
            cy_classic_strobe(seq_init, cy_pattern_fade_delta, cy_pattern_steps_actual); // Strobe the classic cyclotron LEDs by fading them in and out
            break;
        case 10: // Classic 4 Fade Out (Powerdown)
            done = cy_classic_all_fade(seq_init, cy_pattern_fade_delta, true); // Fade out by lowering the intensity in a controlled manor until out
            break;
        case 11: // Classic 4 Fade Out (Powerup)
            done = cy_classic_all_fade(seq_init, cy_pattern_fade_delta, false); // Fade in by raising the intensity in a controlled manor until fully on
            break;
        case 12: // Display LED count
            if (seq_init) {
                clear_strip(&cyclotron_strip);
                for (int i = 0; i < cyclotron_strip.num_pixels; i++) {
                    uint16_t hue = (i * 360) / cyclotron_strip.num_pixels;
                    cyclotron_strip.grbx[i] = hsv_to_grb(hue, 255, 255);
                }
            } else {
                // Do nothing, just let the cycle expire. The main loop will clear the strip.
            }
            break;
        default:
            break; // do nothing if the pattern number is not valid
    }
    if (done) {
        cy_pattern_running = false; // stop the pattern running
    }
}

// This function is intended to be called from inside the ISR to enable pattern updates and control
void cy_pattern_isr_ctrl() {
    if (!cy_pattern_running) return; // if the pattern is not running, do nothing
    if (cy_pattern_init) {
        cy_pattern_run(cy_pattern_num, true); // run the pattern control function
        start_xfer_cyclotron(); // start a transfer for cyclotron
        cy_pattern_init = false; // reset the pattern initialization flag
        cy_pattern_ms_count = 0; // reset the ms counter
    } else {
        cy_pattern_ms_count += 4*256; // increment the count by 4ms (scaled by 256)
        // is it time to update a running pattern?
        if (cy_pattern_ms_count >= cy_pattern_ms_sleep) {
            // time to update the pattern
            cy_pattern_ms_count -= cy_pattern_ms_sleep; // reset the ms counter (ish)
            cy_pattern_run(cy_pattern_num, false); // run the pattern control function
            start_xfer_cyclotron(); // start a transfer for cyclotron

            // see if there is a new pattern speed  only if heat?
        
            // check to see if the pattern cycle count is used
            if (cy_pattern_cycle_num > 0) {
                cy_pattern_cycle_num--; // decrement the cycle number
                if (cy_pattern_cycle_num == 0) {
                    cy_pattern_running = false; // stop the pattern running
                }
            }
        }
    } // else
} // end of cy_pattern_isr_ctrl()