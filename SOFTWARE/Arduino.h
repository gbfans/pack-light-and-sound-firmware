// Arduino.h — RP2040 shim
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#ifndef HIGH
#  define HIGH 1
#endif
#ifndef LOW
#  define LOW 0
#endif
#ifndef INPUT
#  define INPUT 0x0
#endif
#ifndef OUTPUT
#  define OUTPUT 0x1
#endif
#ifndef INPUT_PULLUP
#  define INPUT_PULLUP 0x2
#endif
#ifndef INPUT_PULLDOWN
#  define INPUT_PULLDOWN 0x3
#endif
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
using boolean = bool;
using byte    = uint8_t;
#else
typedef bool    boolean;
typedef uint8_t byte;
#endif

static inline void pinMode(uint pin, int mode) {
  gpio_init(pin);
  switch (mode) {
    case OUTPUT:        gpio_set_dir(pin, true);  break;
    case INPUT:         gpio_set_dir(pin, false); gpio_disable_pulls(pin); break;
    case INPUT_PULLUP:  gpio_set_dir(pin, false); gpio_pull_up(pin);  break;
    case INPUT_PULLDOWN:gpio_set_dir(pin, false); gpio_pull_down(pin);break;
    default:            gpio_set_dir(pin, false); break;
  }
}
static inline void digitalWrite(uint pin, int v) { gpio_put(pin, v != 0); }
static inline int  digitalRead(uint pin) { return gpio_get(pin) ? HIGH : LOW; }

static inline void delay(unsigned long ms)             { sleep_ms(ms); }
static inline void delayMicroseconds(unsigned long us) { sleep_us(us); }
static inline unsigned long millis()  { return (unsigned long)to_ms_since_boot(get_absolute_time()); }
static inline unsigned long micros()  { return (unsigned long)to_us_since_boot(get_absolute_time()); }
static inline void yield() { tight_loop_contents(); }

// Use CPS so no CMSIS dependency
static inline void noInterrupts() { __asm volatile("cpsid i" ::: "memory"); }
static inline void interrupts()   { __asm volatile("cpsie i" ::: "memory"); }
