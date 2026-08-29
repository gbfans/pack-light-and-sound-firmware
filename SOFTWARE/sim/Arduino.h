// Host-side Arduino compatibility header for the animation simulator.
//
// The simulator runs on a virtual clock so renders are deterministic and
// faster than real time: the frame loop advances g_sim_millis explicitly
// and everything that asks for millis() (the RAMP library, party mode's
// timers) sees simulated time.
#pragma once
#include <cstdint>
#include <math.h>  // the firmware's Arduino shim provides math.h; RAMP relies on it

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern uint32_t g_sim_millis;

inline uint32_t millis() { return g_sim_millis; }
inline void delay(uint32_t ms) { g_sim_millis += ms; }
