/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "klystron_IO_support.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

volatile uint16_t adj_pot[2] = {0, 0};
volatile uint8_t config_dip_sw = 0;
volatile uint8_t user_switches = 0;
volatile uint8_t user_switch_flags = 0;

void read_adj_potentiometers(bool average) {
    static uint16_t multiple_readings[2][4] = {0};
    for (int i = 3; i >= 1; i--) {
        multiple_readings[0][i] = multiple_readings[0][i - 1];
        multiple_readings[1][i] = multiple_readings[1][i - 1];
    }
    adc_select_input(0);
    multiple_readings[0][0] = adc_read();
    adc_select_input(1);
    multiple_readings[1][0] = adc_read();
    adj_pot[0] =
        average ? (multiple_readings[0][0] + multiple_readings[0][1] +
                   multiple_readings[0][2] + multiple_readings[0][3] + 2) >> 2
                : multiple_readings[0][0];
    adj_pot[1] =
        average ? (multiple_readings[1][0] + multiple_readings[1][1] +
                   multiple_readings[1][2] + multiple_readings[1][3] + 2) >> 2
                : multiple_readings[1][0];
}

void init_adc(void) {
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
    read_adj_potentiometers(true);
}

void init_gpio(void) {
    for (int gpio = 6; gpio <= 10; gpio++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
    gpio_init(11);
    gpio_set_dir(11, GPIO_IN);
    gpio_pull_up(11);
    for (int gpio = 13; gpio <= 16; gpio++) {
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
    gpio_init(GPO_NBUSY_TO_WAND);
    gpio_set_dir(GPO_NBUSY_TO_WAND, GPIO_OUT);
    gpio_put(GPO_NBUSY_TO_WAND, 1);
    gpio_init(GPO_VENT_LIGHT);
    gpio_set_dir(GPO_VENT_LIGHT, GPIO_OUT);
    gpio_put(GPO_VENT_LIGHT, 0);
    gpio_init(GPO_MUTE);
    gpio_put(GPO_MUTE, 1);
    gpio_set_dir(GPO_MUTE, GPIO_OUT);
}

void check_dip_switches_isr(void) {
    static uint8_t config_dip_last = 0;
    static uint8_t config_dip_maybe = 0;
    static uint8_t debounce_dip_cnt = 0;
    const uint8_t debounce_dip_done = 10;
    config_dip_maybe = 0;
    for (int gpio = 6; gpio <= 10; gpio++) {
        config_dip_maybe |= (gpio_get(gpio) << (10 - gpio));
    }
    config_dip_maybe = 0x1F & (~config_dip_maybe);
    if (config_dip_maybe != config_dip_sw) {
        if (config_dip_maybe != config_dip_last) {
            debounce_dip_cnt = 0;
            config_dip_last = config_dip_maybe;
        }
        debounce_dip_cnt++;
        if (debounce_dip_cnt >= debounce_dip_done) {
            config_dip_sw = config_dip_maybe;
            debounce_dip_cnt = 0;
        }
    } else {
        debounce_dip_cnt = 0;
    }
}

void check_user_switches_isr(void) {
    static uint8_t config_user_last = 0;
    static uint8_t config_user_maybe = 0;
    static uint8_t debounce_user_cnt = 0;
    const uint8_t debounce_user_done = 15;
    static uint8_t debounce_fire_cnt = 0;
    const uint8_t debounce_fire_found = 12;
    const uint8_t debounce_fire_max = 30;

    if (((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL1_MASK) ||
        (((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL_MASK) &&
         (config_dip_sw & DIP_HEAT_MASK))) {
        if (gpio_get(15) == 0) {
            if (debounce_fire_cnt < 250)
                debounce_fire_cnt++;
            if (debounce_fire_cnt == debounce_fire_found) {
                user_switch_flags |= 0x01;
            } else if (debounce_fire_cnt == debounce_fire_max) {
                user_switch_flags &= ~0x03;
            }
        } else {
            if ((debounce_fire_cnt >= debounce_fire_found) &&
                (debounce_fire_cnt <= debounce_fire_max)) {
                user_switch_flags |= 0x02;
                // Clear the FIRE press (bit 3) since it probably registers
                user_switches &= 0xF7;
            }
            debounce_fire_cnt = 0;
        }
    } else {
        debounce_fire_cnt = 0;
        user_switch_flags &= ~0x03;
    }

    config_user_maybe = gpio_get(11);
    for (int gpio = 13; gpio <= 16; gpio++) {
        config_user_maybe |= (gpio_get(gpio) << (gpio - 13 + 1));
    }

    // Invert and mask to the five valid user switch bits
    config_user_maybe = (~config_user_maybe) & 0x1F;
    if (config_user_maybe != (user_switches & 0x1F)) {

        if (config_user_maybe != config_user_last) {
            debounce_user_cnt = 0;
            config_user_last = config_user_maybe;
        }
        debounce_user_cnt++;
        if (debounce_user_cnt >= debounce_user_done) {
            if ((config_user_maybe & 0x04) != (user_switches & 0x04)) {
                user_switch_flags |= 0x04;
            }
            if (((config_user_maybe & 0x01) == 1) &&
                ((user_switches & 0x01) == 0)) {
                user_switch_flags |= 0x08;
            }
            if (((config_user_maybe & 0x01) == 0) &&
                ((user_switches & 0x01) == 1)) {
                user_switch_flags &= ~0x08;
            }
            user_switches = config_user_maybe;
            debounce_user_cnt = 0;
        }
    } else {
        debounce_user_cnt = 0;
    }
}

bool pack_pu_sw(void) { return (user_switches & 0x01); }

bool pack_pu_req(void) { return (user_switch_flags & 0x08); }

bool pu_sw(void) { return (user_switches & 0x10); }

bool fire_sw(void) {
    return ((user_switches & 0x08) && !(user_switch_flags & 0x01));
}

bool fire_tap(void) { return (user_switch_flags & 0x02); }

bool song_sw(void) { return (user_switches & 0x04); }

bool song_toggle(void) { return (user_switch_flags & 0x04); }

bool vent_sw(void) { return (user_switches & 0x02); }

bool wand_standby_sw(void) { return (!pu_sw() && vent_sw()); }

void clear_fire_tap(void) { user_switch_flags &= ~0x03; }

void clear_song_toggle(void) { user_switch_flags &= ~0x04; }

void clear_pack_pu_req(void) { user_switch_flags &= ~0x08; }

void nsignal_to_wandlights(bool autovent) {
    gpio_put(GPO_NBUSY_TO_WAND, autovent ? 0 : 1);
}

void vent_light_on(bool turn_on) {
    gpio_put(GPO_VENT_LIGHT, turn_on ? 1 : 0);
}

void mute_audio(void) { gpio_put(GPO_MUTE, 1); }

void unmute_audio(void) { gpio_put(GPO_MUTE, 0); }

PackType config_pack_type(void) {
    PackType pack_type = PACK_TYPE_SNAP_RED;
    if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL0_MASK) {
        pack_type = PACK_TYPE_FADE_RED;
    } else if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL1_MASK) {
        pack_type = PACK_TYPE_TVG_FADE;
    } else if ((config_dip_sw & DIP_PACKSEL_MASK) == DIP_PACKSEL_MASK) {
        if (config_dip_sw & DIP_HEAT_MASK) {
            pack_type = PACK_TYPE_AFTER_TVG;
        } else {
            pack_type = PACK_TYPE_AFTERLIFE;
        }
    }
    return pack_type;
}

uint8_t config_cyclotron_dir(void) { return 0; }

