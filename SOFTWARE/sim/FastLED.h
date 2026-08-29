// Host-side FastLED compatibility header for the animation simulator.
//
// The simulator compiles the real firmware animation sources
// (animations.cpp, party_sequences.cpp, ...) unmodified; this header stands
// in for <FastLED.h> on the host and provides just the surface those files
// use: CRGB with the named colors and scaling helpers, CHSV with the
// rainbow conversion, and the fill_solid / fill_rainbow / fadeToBlackBy
// helpers. The simulator's include path lists the sim directory first so
// this file shadows the real library, which only builds for the RP2040.
#pragma once
#include <cstdint>

struct CHSV {
  uint8_t h, s, v;
  CHSV(uint8_t ih = 0, uint8_t is = 0, uint8_t iv = 0) : h(ih), s(is), v(iv) {}
};

static inline uint8_t scale8(uint8_t i, uint8_t scale) {
  return (uint16_t(i) * (1 + uint16_t(scale))) >> 8;
}
static inline uint8_t scale8_video(uint8_t i, uint8_t scale) {
  return ((uint16_t(i) * uint16_t(scale)) >> 8) + ((i && scale) ? 1 : 0);
}

struct CRGB {
  uint8_t r, g, b;

  // Named colors used by the firmware, with FastLED's HTML color values.
  enum HTMLColorCode : uint32_t {
    Black = 0x000000,
    Red = 0xFF0000,
    Green = 0x008000,
    Blue = 0x0000FF,
    White = 0xFFFFFF,
    Orange = 0xFFA500,
  };

  CRGB(uint8_t ir = 0, uint8_t ig = 0, uint8_t ib = 0) : r(ir), g(ig), b(ib) {}
  CRGB(HTMLColorCode code)
      : r((code >> 16) & 0xFF), g((code >> 8) & 0xFF), b(code & 0xFF) {}
  CRGB(const CHSV& hsv) { *this = fromHSV(hsv); }

  bool operator==(const CRGB& o) const {
    return r == o.r && g == o.g && b == o.b;
  }
  bool operator!=(const CRGB& o) const { return !(*this == o); }

  CRGB& nscale8(uint8_t scale) {
    r = scale8(r, scale);
    g = scale8(g, scale);
    b = scale8(b, scale);
    return *this;
  }
  CRGB& nscale8_video(uint8_t scale) {
    r = scale8_video(r, scale);
    g = scale8_video(g, scale);
    b = scale8_video(b, scale);
    return *this;
  }
  CRGB& fadeToBlackBy(uint8_t fadefactor) { return nscale8(255 - fadefactor); }

  // FastLED's hsv2rgb_rainbow: eight 32-step hue sections with the
  // yellow-band balancing the library applies.
  static CRGB fromHSV(const CHSV& hsv) {
    uint8_t hue = hsv.h, sat = hsv.s, val = hsv.v;
    uint8_t offset8 = (hue & 0x1F) << 3;
    uint8_t third = scale8(offset8, 256 / 3);
    uint8_t r, g, b;
    switch (hue >> 5) {
      case 0: r = 255 - third; g = third; b = 0; break;
      case 1: r = 171; g = 85 + third; b = 0; break;
      case 2: {
        uint8_t twothirds = scale8(offset8, ((256 * 2) / 3));
        r = 171 - twothirds; g = 170 + third; b = 0;
        break;
      }
      case 3: r = 0; g = 255 - third; b = third; break;
      case 4: {
        uint8_t twothirds = scale8(offset8, ((256 * 2) / 3));
        r = 0; g = 171 - twothirds; b = 85 + twothirds;
        break;
      }
      case 5: r = third; g = 0; b = 255 - third; break;
      case 6: r = 85 + third; g = 0; b = 171 - third; break;
      default: r = 170 + third; g = 0; b = 85 - third; break;
    }
    if (sat != 255) {
      if (sat == 0) {
        r = g = b = 255;
      } else {
        uint8_t desat = 255 - sat;
        desat = scale8_video(desat, desat);
        r = scale8(r, 255 - desat) + desat;
        g = scale8(g, 255 - desat) + desat;
        b = scale8(b, 255 - desat) + desat;
      }
    }
    if (val != 255) {
      if (val == 0) {
        r = g = b = 0;
      } else {
        val = scale8_video(val, val);
        r = scale8(r, val);
        g = scale8(g, val);
        b = scale8(b, val);
      }
    }
    return CRGB(r, g, b);
  }
};

static inline void fill_solid(CRGB* leds, int num, const CRGB& color) {
  for (int i = 0; i < num; ++i) leds[i] = color;
}

static inline void fill_rainbow(CRGB* leds, int num, uint8_t initial_hue,
                                uint8_t delta_hue) {
  CHSV hsv(initial_hue, 240, 255);
  for (int i = 0; i < num; ++i) {
    leds[i] = CRGB(hsv);
    hsv.h += delta_hue;
  }
}

static inline void fadeToBlackBy(CRGB* leds, int num, uint8_t fadeBy) {
  for (int i = 0; i < num; ++i) leds[i].fadeToBlackBy(fadeBy);
}
