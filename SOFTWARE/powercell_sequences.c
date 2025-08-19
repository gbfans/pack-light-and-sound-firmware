/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: powercell_sequences.c
 * Version 1.0.0
 * Date: 2025-05-01
 *
 * This file is part of the GhostLab42 Support for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 *
 * The powercell sequences are used in the ISR to somewhat autonomously provide sequences on the addressable
 * LED string. No "sleep" commands should be used since these routines should be called inside the ISR and
 * need to execute quickly.
 */

#include "powercell_sequences.h"
#include "addressable_LED_support.h"
#include "pico/stdlib.h"

#define powercell_strip (*get_powercell_strip())
 
volatile uint8_t powercell_seq_num = 0; // sequence number for the powercell
volatile uint32_t powercell_color; // Set during pack color updates

const uint16_t pc_normal_cycles = 16; // may be off if blank is included or not

void pc_normal(bool seq_init, bool include_blank) {
    if (seq_init) {
        clear_strip(&powercell_strip); // clear the powercell vars
        powercell_seq_num = 1; // reset the sequence number
    } else {
        if (powercell_seq_num >= NUM_PIXELS_POWERCELL) {
            powercell_seq_num = include_blank ? 0 : 1; // reset the sequence number
            clear_strip(&powercell_strip); // clear the powercell vars
        } else {
            powercell_seq_num++; // increment the sequence number
        }
    }
    if (powercell_seq_num > 0) powercell_strip.grbx[(powercell_seq_num-1) % NUM_PIXELS_POWERCELL] = powercell_color; // Blue is the normal color for the powercell
}

const uint16_t pc_fill_cycles = NUM_PIXELS_POWERCELL + 1; // Number of steps to fill up the powercell in the startup sequence

bool pc_fill(bool seq_init, bool include_blank) {
    bool seq_done = false; // sequence done flag, defaults to not done
    if (seq_init) {
        powercell_seq_num = 0; // reset the sequence number
        // may want to add a completed indicator
        if (include_blank) {
            for (int i=0; i<NUM_PIXELS_POWERCELL; i++) {
                powercell_strip.grbx[i] = 0; // clear the powercell vars
            }
        }
    }
    powercell_strip.grbx[powercell_seq_num % NUM_PIXELS_POWERCELL] = powercell_color; // fill from the bottom up
    powercell_seq_num++; // increment the sequence number
    if (powercell_seq_num >= NUM_PIXELS_POWERCELL) {
        powercell_seq_num = 0; // reset the sequence number
        seq_done = true; // set the sequence done flag
    }
    return seq_done; // return true if the sequence is complete
}

const uint16_t pc_drain_cycles = NUM_PIXELS_POWERCELL + 1; // May be off by 1 due to the optional seq_init parameter

bool pc_drain(bool seq_init) {
    bool seq_done = true; // sequence done flag, defaults to not done
    if (seq_init) {
        for (int i=0; i<NUM_PIXELS_POWERCELL; i++) {
            powercell_strip.grbx[i] = powercell_color; // fill the powercell vars
        }
        seq_done = false; // set the sequence done flag to false
    } else {
        for (int i=0; i<NUM_PIXELS_POWERCELL-1; i++) {
            powercell_strip.grbx[i] = powercell_strip.grbx[i+1]; // shift the data down
            seq_done = (seq_done && (powercell_strip.grbx[i] == 0));
        }
        powercell_strip.grbx[NUM_PIXELS_POWERCELL-1] = 0; // clear out the top (last) pixel
    }
    return seq_done; // return true if the sequence is complete
}

const uint16_t pc_overheat_cyles = 2; // Just a 2-pattern toggle of LEDs

void pc_overheat() {
    // This pattern turns every other pixel on and off, and just swapps the pixels every time it is updated
    // The pattern is determined by if first pixel is off or on.
    uint8_t offset = (powercell_strip.grbx[0] == 0) ? 1 : 0; //
    for (int i=0; i<NUM_PIXELS_POWERCELL; i++) {
        powercell_strip.grbx[i] = ((i+offset) % 2) ? powercell_color : 0x00; // Either Blue or off, every other pixel
    }
}

const uint16_t pc_startup_cyles = (NUM_PIXELS_POWERCELL*(NUM_PIXELS_POWERCELL + 1)) >> 1; // Number of steps to fill up the powercell in the startup sequence

bool pc_startup(bool seq_init) {
    static uint8_t sub_seq_1 = 0; // local subsequence number for a more complex pattern
    bool seq_done = false; // sequence done flag, defaults to not done
    // safety check to make sure we don't go out of bounds
    if ((powercell_seq_num > NUM_PIXELS_POWERCELL-1)  || (sub_seq_1 > NUM_PIXELS_POWERCELL-1)) {
        powercell_seq_num = 0; // reset the sequence number
        sub_seq_1 = 0; // reset the sequence number
    }
    // Either a start of sequence initialization or a continuation of the sequence
    if (seq_init) {
        // start the sequence with a pixel at the top of the powercell string
        clear_strip(&powercell_strip); // clear the powercell vars
        powercell_strip.grbx[NUM_PIXELS_POWERCELL-1] = powercell_color; // set the top pixel to some color
        powercell_seq_num = NUM_PIXELS_POWERCELL-1; // reset the sequence number
        sub_seq_1 = 1; // reset the sequence number
    } else {
        // Move an isolated pixel down the powercell string until it hits another cell or the bottom
        if (sub_seq_1 == 0) {
            // start each drop with a pixel at the top of the powercell
            powercell_strip.grbx[NUM_PIXELS_POWERCELL-1] = powercell_color; // set the last pixel to the color
            seq_done = (powercell_seq_num == 0); // set the sequence done flag if this is the last pixel to turn on
        } else {
            powercell_strip.grbx[NUM_PIXELS_POWERCELL-1-sub_seq_1] = powercell_strip.grbx[NUM_PIXELS_POWERCELL-sub_seq_1]; // shift the data down
            powercell_strip.grbx[NUM_PIXELS_POWERCELL-sub_seq_1] = 0; // clear out the above pixel
        }
        // update the sequence numbers
        sub_seq_1++; // increment the subsequence number
        if (sub_seq_1 > powercell_seq_num) {
            sub_seq_1 = 0; // reset the subsequence number
            if ((powercell_seq_num) != 0) powercell_seq_num--; // decrement the sequence number but stop at 0
        }
    }
    // check if the sequence is complete
    return (seq_done); // return true if the sequence is complete
}


// powercell pattern control variables
volatile uint16_t pc_pattern_ms_sleep = 0;  // number of ms between running the pattern, scaled by 256
volatile uint32_t pc_pattern_ms_count = 0;  // current number of ms since last pattern was run, scaled by 256
volatile uint8_t  pc_pattern_num = 0;       // current pattern number
volatile bool     pc_pattern_init = false;  // pattern initialization flag
volatile uint16_t pc_pattern_cycle_num = 0; // number of cycles for the pattern, 0 for infinite
volatile bool     pc_pattern_running = 0;   // number of cycles for the pattern, 0 for infinite

/****************************
 * int main() use functions *
 * **************************/  

 // update the values that control the speed - can be dynamically updated for heating affects
void pc_pattern_speed_update(uint16_t ms_cycle) {
    uint16_t temp_ms_sleep = 0; // useful for only updating the pc_pattern_ms_sleep only one time
    // use pattern to calculate several values
    switch (pc_pattern_num) { // based on the current pattern number
        case 0: // Normal
            temp_ms_sleep = (256 * ms_cycle) / pc_normal_cycles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;
        case 1: // Fill
            temp_ms_sleep = (256 * ms_cycle) / pc_fill_cycles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;
        case 2: // Drain
        case 3: // Drain w/ prefill
            temp_ms_sleep = (256 * ms_cycle) / pc_drain_cycles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;
        case 4: // Overheat
            temp_ms_sleep = (256 * ms_cycle) / pc_overheat_cyles; // calculate the step size for the pattern, scaled up by a factor of 256
            break;  
        case 5: // Startup
            temp_ms_sleep = (256 * ms_cycle) / pc_startup_cyles; // calculate the step size for the pattern, scaled up by a factor of 256 
            break;
        default:
            temp_ms_sleep = ms_cycle; // dumb default
            break; // do nothing if the pattern number is not valid
    }
    // minimum value is 4ms, so if the ms sleep is less than that, set it to 4ms
    pc_pattern_ms_sleep = (temp_ms_sleep > 4*256) ? temp_ms_sleep : 4*256; // make sure we keep a minimum value that equates to the 4ms ISR rate
} // end of pc_pattern_speed_update

// This function will set the pattern number, ms sleep time, and cycle number for the powercell pattern control
// It will also start the pattern running
void pc_pattern_config(uint8_t pattern_num, uint16_t ms_cycle, uint16_t cycle_num) {
    // to make sure we do not update the variables in the middle of a pattern update, 
    // stop the running of a currently running pattern
    if (pc_pattern_running) {
        pc_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // these are just pass throughs
    pc_pattern_num = pattern_num; // set the pattern number
    pc_pattern_cycle_num = cycle_num; // set the cycle number

    // use calculate the sleep time between updates
    pc_pattern_speed_update(ms_cycle); // update pc_pattern_ms_sleep

    // this will start a pattern running
    pc_pattern_init = true; // set the pattern initialization flag
    pc_pattern_ms_count = 0; // reset the ms counter
    pc_pattern_running = true; // set the pattern running flag to true, needs to be the last thing to update
}

// This function will stop the pattern running and optionally clear the cyclotron variables    
void pc_pattern_stop(bool clear_vars) {
    // stop the running of any currently running pattern
    if (pc_pattern_running) {
        pc_pattern_running = false; // stop the pattern running
        sleep_ms(5); // wait long enough to make sure the ISR sees not running
    }
    // Optionally force all of the LEDs in the string off
    if (clear_vars) {
        clear_strip(&powercell_strip); // clear the powercell vars
        sleep_ms(5); // wait long enough to finish the transfer and have the LEDs see an end of transmission condition
        start_xfer_powercell(); // start a transfer for powercell
    }
}

// This function will return the status of the pattern running flag
bool pc_pattern_is_running() {
    return pc_pattern_running; // return the pattern running flag
}

/*****************
 * ISR Functions *
 *****************/

// This function will start a pattern running 
// Intended use is only for the pc_pattern_isr_ctrl() function
void pc_pattern_run(uint8_t pattern_num, bool seq_init) {
    // local variables for the pattern control
    bool done = false; // pattern done flag, defaults to not done
    // check the pattern number and run the appropriate sequence
    switch (pattern_num) {
        case 0: // Normal
            pc_normal(seq_init, true); // run the normal sequence
            break;
        case 1: // Fill
            done = pc_fill(seq_init, false); // run the fill sequence
            break;
        case 2: // Drain
            done = pc_drain(false); // run the drain sequence
            break;
        case 3: // Drain with prefill
            done = pc_drain(seq_init); // run the drain sequence
            break;
        case 4: // Overheat
            pc_overheat(); // run the overheat sequence
            break;
        case 5: // Startup
            done = pc_startup(seq_init); // run the startup sequence
            break;
        default:
            break; // do nothing if the pattern number is not valid
    }
    if (done) {
        pc_pattern_running = false; // stop the pattern running
    }
}

// This function is intended to be called from inside the ISR to enable pattern updates and control
void pc_pattern_isr_ctrl() {
    if (!pc_pattern_running) return; // if the pattern is not running, do nothing
    if (pc_pattern_init) {
        pc_pattern_run(pc_pattern_num, true); // run the pattern control function
        start_xfer_powercell(); // start a transfer for powercell
        pc_pattern_init = false; // reset the pattern initialization flag
        pc_pattern_ms_count = 0; // reset the ms counter
    } else {
        pc_pattern_ms_count += 4*256; // increment the count by 4ms
        // is it time to update a running pattern?
        if (pc_pattern_ms_count >= pc_pattern_ms_sleep) {
            // time to update the pattern
            pc_pattern_ms_count -= pc_pattern_ms_sleep; // reset the ms counter (ish)
            pc_pattern_run(pc_pattern_num, false); // run the pattern control function
            start_xfer_powercell(); // start a transfer for powercell
 
            // see if there is a new pattern speed  only if heat?
        
           // check to see if the pattern cycle count is used
            if (pc_pattern_cycle_num > 0) {
                pc_pattern_cycle_num--; // decrement the cycle number
                if (pc_pattern_cycle_num == 0) {
                    pc_pattern_running = false; // stop the pattern running
                }
            }
        }
    } // else
} // end of pc_pattern_isr_ctrl()