/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 *
 * Title: klystron.c
 * Version 1.0.0
 * Date: 2025-07-21
 *
 * This file is top level code for the Raspberry Pi Pico
 * based GBFans.com Pack Light and Sound Firmware
 */

// Raspberry Pi Pico SDK includes
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

/*************************************
 * Addressable LED Support Functions *
 *************************************/
#include "addressable_LED_support.h" // support for addressable LEDs functions
#include "powercell_sequences.h" // support for powercell sequence functions
#include "cyclotron_sequences.h" // support for cyclotron sequence functions
#include "future_sequences.h" // support for future sequence functions
#include "party_sequences.h" // support for party mode sequences

/*************************
 * ADC and GPIOFunctions *
 *************************/
#include "klystron_IO_support.h" // support for future sequence functions

/**************************
 * Sound module functions *
 **************************/

/************************
 * Board Test Functions *
 ************************/
#include "board_test.h" // Board test functions header file

#include "heat.h"
#include "monster.h"
#include "led_patterns.h"
#include "sound.h"
#include "pack_state.h"
#include "monitors.h"
#include "pack_config.h"

/********************
 * Timer management *
 ********************/
/**
 * @brief Repeating timer interrupt handler.
 *
 * Debounces inputs, advances LED patterns, and updates heat and monster
 * timers each tick.
 */
bool pack_timer_isr(struct repeating_timer *t) {
    check_dip_switches_isr();
    check_user_switches_isr();
    pc_pattern_isr_ctrl();
    cy_pattern_isr_ctrl();
    fr_pattern_isr_ctrl();
    party_mode_run();
    heat_isr();
    monster_isr();
    return true;
}

/**
 * @brief Initialize the repeating timer used for pack updates.
 */
void init_pack_timer(void) {
    static struct repeating_timer timer;
    add_repeating_timer_ms(pack_isr_interval_ms, pack_timer_isr, NULL, &timer);
}







/*************************************************************
 * Main function
 *************************************************************/             
int main(void) {
    static bool board_test_done = false;

    // initializations
    init_gpio(); // initialize the GPIOs for the inputs and outputs
    init_adc(); // Initialize the ADC for the two potentiometers
    init_leds(); // initialize the LED variables, setup the PIOs and DMAs and then clear the actual LEDs
    init_pack_timer(); // initialize the repeating timer

    // setup the ring size
    ring_monitor(); // look at ADJ1 to determine the user selected ring size

    // initialize the signal to the wand lights
    nsignal_to_wandlights(false);  // Should only be low during autovent

    // initialize the sound module
    // May need to adjust the wait times between these setup commands
    cy_afterlife_init();
    sound_startup();
    
    // Only on initial powerup, check for a board test if all DIP switches are on and fire and song are both activated
    if ( !board_test_done && (config_dip_sw == (DIP_PACKSEL_MASK | DIP_HEAT_MASK | DIP_MONSTER_MASK | DIP_HUM_MASK )) && fire_sw() && song_sw() ) {
        board_test(); // run the board test
        clear_song_toggle(); // Clear the song toggle flag that gets set during testing
        board_test_done = true;
    }

    pack_state_init();

    while (true) {
        pack_state_process();
    }

    return 0;
}
