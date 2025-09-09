#ifndef ANIMATION_H
#define ANIMATION_H

#include <FastLED.h>
#include <stdint.h>

// Forward declaration
class Animation;

/**
 * @brief Configuration for an animation.
 * @details This struct holds the parameters that control an animation's
 *          behavior, such as color, speed, and brightness.
 */
struct AnimationConfig {
    CRGB* leds = nullptr;
    int num_leds = 0;
    CRGB color = CRGB::Black;
    uint16_t speed = 1000; // Default speed in ms
    uint8_t brightness = 255;
    bool clockwise = true;
    uint16_t fade_amount = 0;
    uint16_t steps = 0;
    bool bounce = false;
};

/**
 * @brief Base class for all animations.
 * @details This class defines the interface for all animations. Each
 *          animation should derive from this class and implement the
 *          virtual methods.
 */
#include "libs/RAMP/Ramp.h"
#include "libs/RAMP/RampCRGB.h"

class Animation {
  public:
    Animation() : color_ramp(CRGB::Black), speed_ramp(0), brightness_ramp(0) {}
    virtual ~Animation() {}

    virtual void start(const AnimationConfig &config) {
        this->config = config;
        this->color_ramp.go(config.color, 0);
        this->speed_ramp.go(config.speed, 0);
        this->brightness_ramp.go(config.brightness, 0);
    }

    virtual void setColor(CRGB color, uint32_t duration, ramp_mode mode = LINEAR) {
        color_ramp.go(color, duration, mode);
    }

    virtual void setSpeed(uint16_t speed, uint32_t duration, ramp_mode mode = LINEAR) {
        speed_ramp.go(speed, duration, mode);
    }

    virtual void setBrightness(uint8_t brightness, uint32_t duration, ramp_mode mode = LINEAR) {
        brightness_ramp.go(brightness, duration, mode);
    }

    virtual void update(uint32_t dt) {
        // The Ramp library uses millis(), so we don't need dt here.
        // This is not ideal, but I will stick with it for now to avoid
        // modifying the library too much.
        color_ramp.update();
        speed_ramp.update();
        brightness_ramp.update();
    }

    virtual bool isDone() = 0;

  protected:
    AnimationConfig config;
    RampCRGB color_ramp;
    rampFloat speed_ramp;
    rampFloat brightness_ramp;
};

#endif // ANIMATION_H
