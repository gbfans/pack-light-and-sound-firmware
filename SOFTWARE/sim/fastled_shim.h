#pragma once
#include <cstdint>
#include <functional>
#include <vector>

struct CRGB {
  uint8_t r, g, b;
  CRGB(uint8_t R=0, uint8_t G=0, uint8_t B=0) : r(R), g(G), b(B) {}

  // Mimic the in-place scaling behaviour of FastLED's CRGB::nscale8.
  // Many animations rely on calling leds[i].nscale8(x) to fade pixels
  // between frames. The previous implementation returned a new value
  // without modifying the original colour, which caused pixels to stay
  // lit until explicitly overwritten. Implementing the in-place variant
  // restores the expected persistence semantics.
  CRGB& nscale8(uint8_t scale) {
    r = (uint16_t(r) * scale) >> 8;
    g = (uint16_t(g) * scale) >> 8;
    b = (uint16_t(b) * scale) >> 8;
    return *this;
  }

  // Const variant returning a scaled copy, matching FastLED's API.
  CRGB nscale8(uint8_t scale) const {
    CRGB tmp(*this);
    tmp.nscale8(scale);
    return tmp;
  }

  // FastLED also exposes nscale8_video. This version ensures that non-zero
  // colours don't fade completely to black when scaled by a non-zero
  // factor, matching the behaviour of the hardware implementation.
  CRGB& nscale8_video(uint8_t scale) {
    uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
    r = (r == 0) ? 0 : ((uint16_t(r) * scale) >> 8) + nonzeroscale;
    g = (g == 0) ? 0 : ((uint16_t(g) * scale) >> 8) + nonzeroscale;
    b = (b == 0) ? 0 : ((uint16_t(b) * scale) >> 8) + nonzeroscale;
    return *this;
  }

  // Convenience wrapper matching FastLED's CRGB::fadeToBlackBy().
  // This delegates to nscale8 using (255 - fadefactor) to reduce the
  // brightness of each colour channel in place.
  CRGB& fadeToBlackBy(uint8_t fadefactor) {
    return nscale8(255 - fadefactor);
  }

  static const CRGB Black;
};

inline const CRGB CRGB::Black = CRGB(0, 0, 0);

namespace FastLED {
  inline CRGB* g_leds = nullptr;
  inline int g_nleds = 0;
  inline uint8_t g_brightness = 255;

  template <typename... Args>
  inline void addLeds(CRGB* leds, int n) {
    g_leds = leds;
    g_nleds = n;
  }

  inline void setBrightness(uint8_t b) { g_brightness = b; }

  using ShowCallback = std::function<void(const CRGB*, int, uint8_t)>;
  inline ShowCallback g_show_cb = nullptr;

  inline void show() {
    if (g_show_cb) g_show_cb(g_leds, g_nleds, g_brightness);
  }

  inline void clear(bool force=false) {
    if (!g_leds) return;
    for (int i=0;i<g_nleds;++i) g_leds[i] = CRGB::Black;
  }
}

// Helper mirroring FastLED's global fadeToBlackBy function.  It scales each
// LED in the provided array towards black, ensuring animations that rely on
// gradual fading behave consistently in the simulator.
inline void fadeToBlackBy(CRGB* leds, int numLeds, uint8_t fadeBy) {
  for (int i = 0; i < numLeds; ++i) {
    leds[i].fadeToBlackBy(fadeBy);
  }
}
