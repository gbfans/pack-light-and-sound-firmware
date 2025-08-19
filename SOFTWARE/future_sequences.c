/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: future_sequences.c
 * Version 1.0.0
 * Date: 2025-06-06
 *
 * This file is part of the GhostLab42 Support for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 *
 * The future sequences are used in the ISR to somewhat autonomously provide sequences on the addressable
 * LED string. No "sleep" commands should be used since these routines should be called inside the ISR and
 * need to execute quickly.
 */

#include "future_sequences.h"
#include "addressable_LED_support.h"
#include "pico/stdlib.h"

#define future_strip (*get_future_strip())
 
volatile uint8_t future_seq_num = 0; // sequence number for the future
volatile uint32_t future_color; // Set during pack color updates

const uint16_t fr_strobe_cycles = 4; // Just two patterns to strobe between, but the pattern is controlled by the strobe_on and strobe_off parameters

void fr_strobe(bool seq_init, uint8_t strobe_on, uint8_t strobe_off) {
    // This pattern is a strobe that turns on and off the future LEDs
    if (seq_init) {
        for(int i=0; i<NUM_PIXELS_FUTURE; i++) {
            future_strip.grbx[i] = future_color; // turn on all of the future LEDs
        }
        future_seq_num = 0; // reset the sequence number
    } else {
        future_seq_num++; // increment the sequence number;
        if (future_seq_num == strobe_on){
            // turn off all of the future LEDs
            clear_strip(&future_strip); // clear the future vars
        } else if (future_seq_num >= strobe_on + strobe_off) {
            // turn on all of the future LEDs
            for(int i=0; i<NUM_PIXELS_FUTURE; i++) {
                future_strip.grbx[i] = future_color; // turn on all of the future LEDs
            }
            future_seq_num = 0; // reset the sequence number    
        } 
    }
}

const uint16_t fr_rotate_cycles = NUM_PIXELS_FUTURE; // Number of steps to fill up the powercell in the startup sequence

void fr_rotate(bool seq_init, bool clockwise) {
    // This pattern rotates the future LEDs by shifting them around in one direction or the other
    int32_t temp_hold = 0; // temporary variable to hold the value of the first pixel
    if (seq_init) {
        for(int i=0; i<NUM_PIXELS_FUTURE-1; i++) {
            if ( ((i % 4) == 0) ) {
                future_strip.grbx[i] = future_color; // turn on some of the future LEDs
            } else {
                future_strip.grbx[i] = 0; // turn off some of the future LEDs
            }
        future_seq_num = 0; // reset the sequence number
        }
    } else {
        future_seq_num++; // increment the sequence number
        if (future_seq_num >= NUM_PIXELS_FUTURE) {
            future_seq_num = 0; // reset the sequence number
        }
        if (clockwise) {
            // shift the data to the right
            temp_hold = future_strip.grbx[NUM_PIXELS_FUTURE-1]; // hold the last pixel value
            for (int i=NUM_PIXELS_FUTURE-1; i>0; i--) {
                future_strip.grbx[i] = future_strip.grbx[i-1]; // shift the data down
            }
            future_strip.grbx[0] = temp_hold; // set the first pixel to the color that was from the last pixel
        } else {
            // shift the data to the left
            temp_hold = future_strip.grbx[0]; // hold the first pixel value
            for (int i=0; i<NUM_PIXELS_FUTURE-1; i++) {
                future_strip.grbx[i] = future_strip.grbx[i+1]; // shift the data down
            }
            future_strip.grbx[NUM_PIXELS_FUTURE-1] = temp_hold; // set the last pixel to the color that was in the first pixel
        }
    }
}
    
// future pattern control variables
volatile uint16_t fr_pattern_ms_sleep = 0; // number of ms between running the pattern
volatile uint32_t fr_pattern_ms_count = 0; // current number of ms since last pattern was run
volatile uint8_t  fr_pattern_num = 0; // current pattern number
volatile bool     fr_pattern_init = false; // pattern initialization flag
volatile uint16_t fr_pattern_cycle_num = 0; // number of cycles for the pattern, 0 for infinite
volatile bool     fr_pattern_running = 0; // number of cycles for the pattern, 0 for infinite


/****************************
 * int main() use functions *
 * **************************/  

// This function will set the pattern number, ms sleep time, and cycle number for the future pattern control
// It will also start the pattern running
void fr_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num) {
    // to make sure we do not update the variables in the middle of a pattern update, 
    // stop the running of a currently running pattern
    if (fr_pattern_running) {
        fr_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // these are just pass throughs
    fr_pattern_num = pattern_num; // set the pattern number
    fr_pattern_ms_sleep = ms_cycle; // set the ms sleep time
    fr_pattern_cycle_num = cycle_num; // set the cycle number

    // use pattern to calculate several values
    switch (pattern_num) {
        case 0: // Strobe
            fr_pattern_ms_sleep = (256 * ms_cycle) / fr_strobe_cycles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;
        case 1: // Rotate CW
        case 2: // Rotate CCW
            fr_pattern_ms_sleep = (256 * ms_cycle) / fr_rotate_cycles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;
        default:
            break; // do nothing if the pattern number is not valid
    }
    // minimum value is 4ms, so if the ms sleep is less than that, set it to 4ms
    fr_pattern_ms_sleep = (fr_pattern_ms_sleep > 4*256) ? fr_pattern_ms_sleep : 4*256; // make sure we keep a minimum value that equates to the 4ms ISR rate

    // this will start a pattern running
    fr_pattern_init = true; // set the pattern initialization flag
    fr_pattern_ms_count = 0; // reset the ms counter
    fr_pattern_running = true; // set the pattern running flag to true, needs to be the last thing to update
}

// This function will stop the pattern running
void fr_pattern_stop(bool clear_vars) {
    // This function will stop the pattern running and optionally clear the future variables    
    // stop the running of any currently running pattern
    if (fr_pattern_running) {
        fr_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // Optionally force all of the LEDs in the string off
    if (clear_vars) {
        clear_strip(&future_strip); // clear the future vars
        sleep_ms(5); // wait long enough to finish the transfer and have the LEDs see an end of transmission condition
        start_xfer_future(); // start a transfer for future
    }
}

// This function will return the status of the pattern running flag
bool fr_pattern_is_running() {
    return fr_pattern_running; // return the pattern running flag
}

/*****************
 * ISR Functions *
 *****************/

// This function will start a pattern running 
// Intended use is only for the fr_pattern_isr_ctrl() function
void fr_pattern_run(uint8_t pattern_num, bool seq_init) {
    // local variables for the pattern control
    bool done = false; // pattern done flag, defaults to not done
    // check the pattern number and run the appropriate sequence
    //so far, all sequences run forever, so must be stopped by a fr_pattern_stop call
    switch (pattern_num) {
        case 0: // Strobe
            fr_strobe(seq_init, 1, 3); // Strobe on for 1 cycle and off for 3 cycles
            break;
        case 1: // Rotate CW
            fr_rotate(seq_init, true); // Rotate a single "off" pixel around the future string in CW direction
            break;
        case 2: // Rotate CCW
            fr_rotate(seq_init, false); // Rotate a single "off" pixel around the future string in CCW direction
            break;
         default:
            break; // do nothing if the pattern number is not valid
    }
    if (done) {
        fr_pattern_running = false; // stop the pattern running
    }
}

// This function is intended to be called from inside the ISR to enable pattern updates and control
void fr_pattern_isr_ctrl() {
    if (!fr_pattern_running) return; // if the pattern is not running, do nothing
    if (fr_pattern_init) {
        fr_pattern_run(fr_pattern_num, true); // run the pattern control function
        start_xfer_future(); // start a transfer for future
        fr_pattern_init = false; // reset the pattern initialization flag
        fr_pattern_ms_count = 0; // reset the ms counter
    } else {
        fr_pattern_ms_count += 4*256; // increment the count by 4ms
        // is it time to update a running pattern?
        if (fr_pattern_ms_count >= fr_pattern_ms_sleep) {
            // is it time to update a running pattern?
            fr_pattern_ms_count -= fr_pattern_ms_sleep; // reset the ms counter (ish)
            fr_pattern_run(fr_pattern_num, false); // run the pattern control function
            start_xfer_future(); // start a transfer for future
            // check to see if the pattern cycle count is used
            if (fr_pattern_cycle_num > 0) {
                fr_pattern_cycle_num--; // decrement the cycle number
                if (fr_pattern_cycle_num == 0) {
                    fr_pattern_running = false; // stop the pattern running
                }
            }
        }
    } // else
} // end of fr_pattern_isr_ctrl()