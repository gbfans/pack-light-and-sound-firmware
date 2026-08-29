#ifndef ACTION_H
#define ACTION_H

#include <stdint.h>
#include "animation.h"
#include <memory>

class AnimationController; // Forward declaration

class Action {
public:
    virtual ~Action() {}
    virtual void start(AnimationController* controller) {
        this->controller = controller;
    }
    virtual bool update(uint32_t dt) = 0; // Returns true when done

protected:
    AnimationController* controller;
};

class PlayAnimationAction : public Action {
public:
    PlayAnimationAction(std::unique_ptr<Animation> anim, const AnimationConfig& config)
        : animation(std::move(anim)), config(config) {}

    void start(AnimationController* controller) override;
    bool update(uint32_t dt) override;

private:
    std::unique_ptr<Animation> animation;
    AnimationConfig config;
};

class ChangeColorAction : public Action {
public:
    ChangeColorAction(CRGB color, uint32_t duration, ramp_mode mode = LINEAR)
        : color(color), duration_ms(duration), mode(mode) {}

    void start(AnimationController* controller) override;
    bool update(uint32_t dt) override;

private:
    CRGB color;
    uint32_t duration_ms;
    ramp_mode mode;
};

class ChangeSpeedAction : public Action {
public:
    ChangeSpeedAction(uint16_t speed, uint32_t duration, ramp_mode mode = LINEAR)
        : speed(speed), duration_ms(duration), mode(mode) {}

    void start(AnimationController* controller) override;
    bool update(uint32_t dt) override;

private:
    uint16_t speed;
    uint32_t duration_ms;
    ramp_mode mode;
};

#endif // ACTION_H
