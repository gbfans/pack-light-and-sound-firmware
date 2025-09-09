#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "animation.h"
#include "party_sequences.h"

/**
 * @brief Rainbow feedback animation for ADJ1 potentiometer changes.
 *        The first \c N LEDs display a rotating rainbow while any remaining
 *        pixels are forced to black. The animation automatically times out
 *        after a fixed duration.
 */
class FeedbackRainbowAnimation : public Animation {
public:
    explicit FeedbackRainbowAnimation(uint32_t duration_ms) : duration_ms(duration_ms) {}
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
    void updateConfig(const AnimationConfig& config, uint32_t extend_ms);

private:
    uint32_t duration_ms;
    uint32_t elapsed_ms = 0;
};

class PartyRainbowFadeAnimation : public Animation {
public:
    PartyRainbowFadeAnimation(PartyModeState* state) : state(state) {}
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    PartyModeState* state;
};


class PartyRandomSparkleAnimation : public Animation {
public:
    PartyRandomSparkleAnimation(PartyModeState* state, uint8_t strip_index) : state(state), strip_index(strip_index) {}
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    PartyModeState* state;
    uint8_t strip_index;
};

class BeatMeterAnimation : public Animation {
public:
    BeatMeterAnimation(PartyModeState* state) : state(state) {}
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    PartyModeState* state;
};

class ShiftRotateAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
    uint8_t offset = 0;
};

class RotateFadeAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t rotation_index = 0;
    uint8_t prev_rotation_index = 0;
    uint16_t fade_value = 255;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class SlimeAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t rotation_index = 0;
    uint16_t sub_seq1 = 0;
    uint16_t fade_value = 0;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class CylonFadeOutAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint16_t pos_accum = 0;
    uint16_t fade_value = 0;
    uint8_t seq_num = 0;
    bool done = false;
    uint32_t time_since_last_update = 0;
};

class ScrollAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t seq_num = 0;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class FillAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t seq_num = 0;
    bool done = false;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class DrainAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    bool done = false;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class StrobeAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class WaterfallAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t seq_num = 0;
    uint8_t sub_seq_1 = 0;
    bool done = false;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class CylonAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    int8_t direction = 1;
    uint8_t position = 0;
    uint32_t time_since_last_update = 0;
};

class RotateAnimation : public Animation {
public:
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint8_t rotation_index = 0;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

class FadeAnimation : public Animation {
public:
    FadeAnimation(bool fade_out) : fade_out(fade_out) {}
    void start(const AnimationConfig& config) override;
    void update(uint32_t dt) override;
    bool isDone() override;
private:
    uint16_t fade_value = 0;
    bool fade_out;
    bool done = false;
    uint32_t time_since_last_update = 0;
    uint16_t step_time_ms = 0;
};

#endif // ANIMATIONS_H
