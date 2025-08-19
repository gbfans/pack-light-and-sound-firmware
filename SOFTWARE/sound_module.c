/*
 * Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 * Licensed under the MIT License. See LICENSE file for details.
 */

/**
 * @file sound_module.c
 * @brief Low-level driver for the serial sound module.
 */

#include "sound_module.h"
#include "klystron_IO_support.h"
#include "pack_config.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

void sound_init(void) {
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
    uart_init(uart0, pack_sound_baud_rate);
    gpio_init(pack_sound_busy_pin);
    gpio_set_dir(pack_sound_busy_pin, GPIO_IN);
    gpio_pull_up(pack_sound_busy_pin);
}

void sound_start(uint8_t sound_index) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x0F');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, sound_index);
    uart_putc_raw(uart0, '\xEF');
}

void sound_wait_til_end(bool fire, bool shutdown) {
    int i = 0;
    while ((gpio_get(pack_sound_busy_pin) == (pack_sound_busy_level ^ 1)) &&
           (i < 20)) {
        i++;
        sleep_ms(10);
    }
    while (gpio_get(pack_sound_busy_pin) == pack_sound_busy_level) {
        sleep_ms(10);
        if (fire && fire_sw())
            break;
        if (shutdown && !pu_sw() && !pack_pu_sw() && !wand_standby_sw())
            break;
    }
}

bool sound_is_playing(void) {
    return (gpio_get(pack_sound_busy_pin) == pack_sound_busy_level);
}

void sound_stop(void) {
    if (gpio_get(pack_sound_busy_pin) == pack_sound_busy_level) {
        uart_puts(uart0, "\x7E\xFF\x06");
        uart_putc_raw(uart0, '\x16');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\xEF');
    }
}

void sound_pause(void) {
    if (gpio_get(pack_sound_busy_pin) == pack_sound_busy_level) {
        uart_puts(uart0, "\x7E\xFF\x06");
        uart_putc_raw(uart0, '\x0E');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\x00');
        uart_putc_raw(uart0, '\xEF');
    }
}

void sound_resume(void) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x0D');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\xEF');
}

void sound_repeat(uint8_t sound_index) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x08');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, sound_index + 1);
    uart_putc_raw(uart0, '\xEF');
}

void sound_volume(uint8_t volume_level) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x06');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    if (volume_level > pack_sound_max_volume)
        volume_level = pack_sound_max_volume;
    uart_putc_raw(uart0, volume_level);
    uart_putc_raw(uart0, '\xEF');
}

