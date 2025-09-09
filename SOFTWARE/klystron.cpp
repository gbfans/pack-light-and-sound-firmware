/**
 * @file klystron.cpp
 * @brief Main entry point for the Klystron firmware.
 * @details This file contains the `main` function, which initializes all
 *          hardware and software subsystems, and then enters an infinite loop
 *          to run the main pack state machine. It also contains the top-level
 *          repeating timer ISR that drives all time-based events.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

// Raspberry Pi Pico SDK includes
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Firmware includes
#include "addressable_LED_support.h"
#include "powercell_sequences.h"
#include "cyclotron_sequences.h"
#include "animation_controller.h"
#include "future_sequences.h"
#include "party_sequences.h"
#include "klystron_IO_support.h"
#include "board_test.h"
#include "heat.h"
#include "monster.h"
#include "led_patterns.h"
#include "sound.h"
#include "pack_state.h"
#include "monitors.h"
#include "pack_config.h"

// Global animation controllers
AnimationController g_powercell_controller;
AnimationController g_cyclotron_controller;
AnimationController g_future_controller;

/**
 * @brief Repeating timer interrupt handler.
 * @details This function is the main heartbeat of the firmware. It's called
 *          by a repeating timer every `pack_isr_interval_ms`. It is responsible
 *          for polling inputs, advancing all animation patterns, and updating
 *          timers.
 * @param t Pointer to the repeating_timer structure.
 * @return true to continue the timer, false to stop it.
 */
bool pack_timer_isr(struct repeating_timer *t) {
    // Poll hardware inputs
    check_dip_switches_isr();
    check_user_switches_isr();

    // Ensure any LEDs above the active count remain dark before animations run.
    // mask_cyclotron_leds();

    // Advance animation patterns
    g_powercell_controller.update(pack_isr_interval_ms);
    g_cyclotron_controller.update(pack_isr_interval_ms);
    g_future_controller.update(pack_isr_interval_ms);
    party_mode_run();

    // Update timers and other modules
    heat_isr();
    monster_isr();

    // Push updated LED state to the physical strips
    show_leds();
    return true;
}

/**
 * @brief Initializes the repeating timer used for pack updates.
 */
void init_pack_timer(void) {
    static struct repeating_timer timer;
    add_repeating_timer_ms(pack_isr_interval_ms, pack_timer_isr, NULL, &timer);
}

/**
 * @brief Main application entry point.
 * @details This function performs all one-time initializations for hardware
 *          and software modules. It checks for the special board test mode
 *          entry condition. If not in test mode, it initializes the main pack
 *          state machine and enters an infinite loop to process it.
 * @return 0 on successful execution (though it should never return).
 */
int main(void) {
    static bool board_test_done = false;

    // Hardware and software initializations
    init_gpio();
    init_adc();
    init_leds();
    init_pack_timer();

    // Set initial cyclotron ring size from the potentiometer
    ring_monitor();

    // Initialize the signal to the wand lights
    nsignal_to_wandlights(false);

    // Initialize the sound module
    sound_startup();
    
    // Check for board test mode entry condition on initial power-up
    if ( !board_test_done && (config_dip_sw == (DIP_PACKSEL_MASK | DIP_HEAT_MASK | DIP_MONSTER_MASK | DIP_HUM_MASK )) && fire_sw() && song_sw() ) {
        board_test();
        clear_song_toggle(); // Clear the song toggle flag that gets set during testing
        board_test_done = true;
    }

    // Initialize the main state machine
    pack_state_init();

    // Main application loop
    while (true) {
        pack_state_process();
    }

    return 0;
}
