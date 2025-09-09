#include "RampCRGB.h"

RampCRGB::RampCRGB(CRGB initialValue) {
    r.go(initialValue.r);
    g.go(initialValue.g);
    b.go(initialValue.b);
}

void RampCRGB::go(CRGB target, unsigned long duration, ramp_mode mode) {
    r.go(target.r, duration, mode);
    g.go(target.g, duration, mode);
    b.go(target.b, duration, mode);
}

CRGB RampCRGB::update() {
    return CRGB(r.update(), g.update(), b.update());
}

CRGB RampCRGB::getValue() {
    return CRGB(r.getValue(), g.getValue(), b.getValue());
}
