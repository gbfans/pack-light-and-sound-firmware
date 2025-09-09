/**
 * @file sound_module.cpp
 * @brief Implements the low-level driver for the serial sound board.
 * @details This file provides the direct interface for controlling an external
 *          serial sound module (like a DFPlayer Mini). It handles sending
 *          commands for playing, stopping, and managing volume by writing
 *          byte commands over a UART serial connection.
 * @copyright
 *   Copyright (c) 2025 GhostLab42 LLC & GBFans LLC
 *   Licensed under the MIT License. See LICENSE file for details.
 */

#include "sound_module.h"
#include "klystron_IO_support.h"
#include "pack_config.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the serial interface to the sound module.
 * @details Sets up the UART communication on UART0 (GPIO 0 and 1) and
 *          configures the BUSY pin (GPIO 2) as a pulled-up input.
 */
void sound_init(void) {
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
    uart_init(uart0, pack_sound_baud_rate);
    gpio_init(pack_sound_busy_pin);
    gpio_set_dir(pack_sound_busy_pin, GPIO_IN);
    gpio_pull_up(pack_sound_busy_pin);
}

/**
 * @brief Starts playback of a sound by its index number.
 * @details Sends the "play track by index" command sequence over UART.
 * @param sound_index The 1-based index of the sound file to play.
 */
void sound_start(uint8_t sound_index) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x0F');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, sound_index);
    uart_putc_raw(uart0, '\xEF');
}

/**
 * @brief Waits until the current sound finishes playing.
 * @details This is a blocking function that polls the sound module's busy
 *          status pin. It can be configured to abort early if the user triggers
 *          a primary activation or shutdown event.
 * @param fire If true, the wait will abort if the main activation switch is pressed.
 * @param shutdown If true, the wait will abort if a pack shutdown is requested.
 */
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

/**
 * @brief Checks if the sound module is currently playing a sound.
 * @details This is determined by reading the logic level of the sound module's
 *          `BUSY` pin.
 * @return true if audio is playing, false otherwise.
 */
bool sound_is_playing(void) {
    return (gpio_get(pack_sound_busy_pin) == pack_sound_busy_level);
}

/**
 * @brief Stops the currently playing sound immediately.
 * @details Sends the "stop playback" command sequence over UART.
 */
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

/**
 * @brief Pauses playback of the current sound.
 * @details Sends the "pause" command sequence over UART.
 * @note The sound can be resumed from the same position with `sound_resume()`.
 */
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

/**
 * @brief Resumes playback of a previously paused sound.
 * @details Sends the "resume" command sequence over UART.
 */
void sound_resume(void) {
    uart_puts(uart0, "\x7E\xFF\x0G");
    uart_putc_raw(uart0, '\x0D');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\xEF');
}

/**
 * @brief Plays a sound in a continuous loop.
 * @details Sends the "play track in loop" command sequence over UART.
 * @param sound_index The index of the sound file to repeat.
 */
void sound_repeat(uint8_t sound_index) {
    uart_puts(uart0, "\x7E\xFF\x06");
    uart_putc_raw(uart0, '\x08');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, '\x00');
    uart_putc_raw(uart0, sound_index + 1);
    uart_putc_raw(uart0, '\xEF');
}

/**
 * @brief Sets the playback volume level.
 * @details Sends the "set volume" command sequence over UART.
 * @param volume_level The new volume level, clamped to the maximum defined
 *                     in `pack_config`.
 */
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

#ifdef __cplusplus
}
#endif
