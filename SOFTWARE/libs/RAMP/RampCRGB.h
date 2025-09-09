#ifndef RAMPCRGB_H
#define RAMPCRGB_H

#include <FastLED.h>
#include "Ramp.h"

class RampCRGB {
public:
    RampCRGB(CRGB initialValue);
    void go(CRGB target, unsigned long duration, ramp_mode mode = LINEAR);
    CRGB update();
    CRGB getValue();
private:
    rampFloat r, g, b;
};

#endif // RAMPCRGB_H
