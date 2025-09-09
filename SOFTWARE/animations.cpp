#include "animations.h"
#include "addressable_LED_support.h"
#include <FastLED.h>
#include <stdlib.h>
#include "pack_state.h"
#include "cyclotron_sequences.h"

// --- Helper functions for cyclotron-specific animations ---
// These are still needed for RotateAnimation and SlimeAnimation
extern const uint8_t cyc_classic_pos[4][5];
extern volatile uint8_t cyclotron_color_set_size;
extern volatile CRGB cyclotron_color_set[5];

static const uint8_t* get_classic_positions() {
    if (g_cyclotron_led_count == 4) return cyc_classic_pos[0];
    if (g_cyclotron_led_count == 24) return cyc_classic_pos[1];
    if (g_cyclotron_led_count == 32) return cyc_classic_pos[2];
    return cyc_classic_pos[3];
}

static void reset_classic_color_set(const CRGB& color) {
    cyclotron_color_set_size = 1;
    cyclotron_color_set[0].r = color.r;
    cyclotron_color_set[0].g = color.g;
    cyclotron_color_set[0].b = color.b;
    for (int i = 1; i < 5; i++) {
        cyclotron_color_set[i].r = 0;
        cyclotron_color_set[i].g = 0;
        cyclotron_color_set[i].b = 0;
    }
}

static void draw_cylon_eye(CRGB *leds, int num_leds, int center, const CRGB &color) {
    // Use FastLED's fill_solid for clearing to leverage optimized routines.
    fill_solid(leds, num_leds, CRGB::Black);
    if (center < 0 || center >= num_leds) return;
    leds[center] = color;
}

// --- Generic Animation Implementations ---

void ShiftRotateAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    offset = 0;
    CRGB color = this->color_ramp.getValue();
    for (int i = 0; i < config.num_leds; i++) {
        config.leds[i] = ((i % 4) == 0) ? color : CRGB::Black;
    }
    time_since_last_update = 0;
    step_time_ms = this->speed_ramp.getValue() / config.num_leds;
}

void ShiftRotateAnimation::update(uint32_t dt) {
    Animation::update(dt);
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        offset = (offset + (config.clockwise ? 1 : 3)) & 0x3;
        CRGB color = this->color_ramp.getValue();
        for (int i = 0; i < config.num_leds; i++) {
            config.leds[i] = (((i + offset) % 4) == 0) ? color : CRGB::Black;
        }
    }
}
bool ShiftRotateAnimation::isDone() { return false; }

void RotateFadeAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    if (this->config.steps == 0) this->config.steps = 1;
    if (this->config.fade_amount == 0) this->config.fade_amount = 1;
    reset_classic_color_set(this->color_ramp.getValue());
    this->rotation_index = 0;
    this->prev_rotation_index = 0;
    this->fade_value = 255;
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    const uint8_t* positions = get_classic_positions();
    for (int i = 0; i < cyclotron_color_set_size; i++) {
        uint16_t pos = positions[rotation_index] - 1;
        config.leds[(pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = CRGB(cyclotron_color_set[i].r, cyclotron_color_set[i].g, cyclotron_color_set[i].b);
    }
    this->time_since_last_update = 0;
    this->step_time_ms = (this->speed_ramp.getValue() / 4) / this->config.steps;
}

void RotateFadeAnimation::update(uint32_t dt) {
    Animation::update(dt);
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        const uint8_t* positions = get_classic_positions();
        uint16_t prev_pos = positions[prev_rotation_index] - 1;
        uint16_t cur_pos = positions[rotation_index] - 1;
        for (int i = 0; i < cyclotron_color_set_size; i++) {
            CRGB base = CRGB(cyclotron_color_set[i].r, cyclotron_color_set[i].g, cyclotron_color_set[i].b);
            CRGB in_col = base;
            in_col.nscale8_video(fade_value);
            config.leds[(cur_pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = in_col;

            CRGB out_col = base;
            out_col.nscale8_video(255 - fade_value);
            config.leds[(prev_pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = out_col;
        }
        if (fade_value < 255) {
            uint16_t new_fade = fade_value + this->config.fade_amount;
            fade_value = (new_fade > 255) ? 255 : new_fade;
        } else {
            fade_value = 0;
            prev_rotation_index = rotation_index;
            rotation_index = (rotation_index + (this->config.clockwise ? 1 : 3)) % 4;
        }
    }
}
bool RotateFadeAnimation::isDone() { return false; }

void SlimeAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    if (this->config.steps == 0) this->config.steps = 1;
    if (this->config.fade_amount == 0) this->config.fade_amount = 1;
    reset_classic_color_set(this->color_ramp.getValue());
    this->rotation_index = 0;
    this->sub_seq1 = 0;
    this->fade_value = 0;
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    const uint8_t* positions = get_classic_positions();
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < cyclotron_color_set_size; i++) {
            uint16_t pos = positions[j] - 1;
            config.leds[(pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = CRGB(cyclotron_color_set[i].r, cyclotron_color_set[i].g, cyclotron_color_set[i].b);
        }
    }
    this->time_since_last_update = 0;
    this->step_time_ms = (this->speed_ramp.getValue() / 4) / this->config.steps;
}

void SlimeAnimation::update(uint32_t dt) {
    Animation::update(dt);
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        const uint8_t* positions = get_classic_positions();
        sub_seq1++;
        if (fade_value < 255) fade_value += this->config.fade_amount;
        if (fade_value >= 255) fade_value = 255;
        for (int i = 0; i < cyclotron_color_set_size; i++) {
            uint16_t pos = positions[rotation_index] - 1;
            config.leds[(pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds].nscale8(255 - fade_value);
        }
        if (sub_seq1 >= this->config.steps) {
            fade_value = 0;
            sub_seq1 = 0;
            uint16_t prev_pos = positions[rotation_index] - 1;
            for (int i = 0; i < cyclotron_color_set_size; i++) {
                config.leds[(prev_pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = CRGB(cyclotron_color_set[i].r, cyclotron_color_set[i].g, cyclotron_color_set[i].b);
            }
            rotation_index = (rotation_index + (this->config.clockwise ? 1 : 3)) % 4;
        }
    }
}
bool SlimeAnimation::isDone() { return false; }

void CylonFadeOutAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    this->pos_accum = 0;
    this->fade_value = 0;
    this->seq_num = 0;
    this->done = false;
    this->time_since_last_update = 0;
}

void CylonFadeOutAnimation::update(uint32_t dt) {
    Animation::update(dt);
    if (done) return;
    uint32_t delta = this->speed_ramp.getValue();
    pos_accum += (delta * dt) / 16;
    if (pos_accum >= (1 << 14)) {
        config.leds[seq_num % config.num_leds] = CRGB::Black;
        seq_num = (seq_num + (pos_accum >> 14)) % config.num_leds;
        pos_accum &= 0x3FFF;
    }
    CRGB base = this->color_ramp.getValue();
    base.nscale8(255 - (fade_value >> 8));
    config.leds[seq_num % config.num_leds] = base;
    uint32_t new_fade = (uint32_t)fade_value + this->config.fade_amount;
    fade_value = (new_fade > 65535) ? 65535 : (uint16_t)new_fade;
    if (fade_value >= 65535) {
        done = true;
        fill_solid(config.leds, config.num_leds, CRGB::Black);
    }
}
bool CylonFadeOutAnimation::isDone() { return done; }

void ScrollAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    this->seq_num = 0;
    this->time_since_last_update = 0;
}

void ScrollAnimation::update(uint32_t dt) {
    Animation::update(dt);
    uint16_t step_time_ms = this->speed_ramp.getValue() / config.num_leds;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        if (seq_num == 0) fill_solid(config.leds, config.num_leds, CRGB::Black);
        config.leds[seq_num] = this->color_ramp.getValue();
        seq_num = (seq_num + 1) % config.num_leds;
    }
}
bool ScrollAnimation::isDone() { return false; }

void FillAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    this->seq_num = 0;
    this->done = false;
    this->time_since_last_update = 0;
}

void FillAnimation::update(uint32_t dt) {
    Animation::update(dt);
    if (done) return;
    uint16_t step_time_ms = this->speed_ramp.getValue() / config.num_leds;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        config.leds[seq_num] = this->color_ramp.getValue();
        seq_num++;
        if (seq_num >= config.num_leds) done = true;
    }
}
bool FillAnimation::isDone() { return done; }

void DrainAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    fill_solid(config.leds, config.num_leds, this->color_ramp.getValue());
    this->done = false;
    this->time_since_last_update = 0;
}

void DrainAnimation::update(uint32_t dt) {
    Animation::update(dt);
    if (done) return;
    uint16_t step_time_ms = this->speed_ramp.getValue() / config.num_leds;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        bool all_black = true;
        for (int i = 0; i < config.num_leds - 1; i++) {
            config.leds[i] = config.leds[i + 1];
            if (config.leds[i] != CRGB::Black) all_black = false;
        }
        config.leds[config.num_leds - 1] = CRGB::Black;
        if (all_black && config.leds[config.num_leds - 1] == CRGB::Black) done = true;
    }
}
bool DrainAnimation::isDone() { return done; }

void StrobeAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    this->time_since_last_update = 0;
}

void StrobeAnimation::update(uint32_t dt) {
    Animation::update(dt);
    uint16_t step_time_ms = this->speed_ramp.getValue() / 2;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        uint8_t offset = (config.leds[0] == CRGB::Black) ? 1 : 0;
        for (int i = 0; i < config.num_leds; i++) {
            config.leds[i] = ((i + offset) % 2) ? this->color_ramp.getValue() : CRGB::Black;
        }
    }
}
bool StrobeAnimation::isDone() { return false; }

void WaterfallAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    config.leds[config.num_leds - 1] = this->color_ramp.getValue();
    this->seq_num = config.num_leds - 1;
    this->sub_seq_1 = 1;
    this->done = false;
    this->time_since_last_update = 0;
}

void WaterfallAnimation::update(uint32_t dt) {
    Animation::update(dt);
    if (done) return;
    const uint16_t total_steps = (config.num_leds * (config.num_leds + 1)) >> 1;
    uint16_t step_time_ms = this->speed_ramp.getValue() / total_steps;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        if (sub_seq_1 == 0) {
            config.leds[config.num_leds - 1] = this->color_ramp.getValue();
            if (seq_num == 0) done = true;
        } else {
            config.leds[config.num_leds - 1 - sub_seq_1] = config.leds[config.num_leds - sub_seq_1];
            config.leds[config.num_leds - sub_seq_1] = CRGB::Black;
        }
        sub_seq_1++;
        if (sub_seq_1 > seq_num) {
            sub_seq_1 = 0;
            if (seq_num != 0) seq_num--;
        }
    }
}
bool WaterfallAnimation::isDone() { return done; }

void CylonAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    position = 0;
    direction = 1;
    time_since_last_update = 0;
    draw_cylon_eye(config.leds, config.num_leds, position, this->color_ramp.getValue());
}

void CylonAnimation::update(uint32_t dt) {
    Animation::update(dt);
    time_since_last_update += dt;
    if (time_since_last_update < this->speed_ramp.getValue()) return;
    time_since_last_update = 0;
    if (config.bounce && config.num_leds > 1) {
        position += direction;
        if (position >= config.num_leds - 1 || position <= 0) direction = -direction;
    } else if (config.num_leds > 0) {
        position = (position + 1) % config.num_leds;
    }

    draw_cylon_eye(config.leds, config.num_leds, position, this->color_ramp.getValue());
}
bool CylonAnimation::isDone() { return false; }

void RotateAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    reset_classic_color_set(this->color_ramp.getValue());
    this->rotation_index = 0;
    fill_solid(config.leds, config.num_leds, CRGB::Black);
    this->time_since_last_update = 0;
}

void RotateAnimation::update(uint32_t dt) {
    Animation::update(dt);
    uint16_t step_time_ms = this->speed_ramp.getValue() / 4;
    time_since_last_update += dt;

    if (step_time_ms != 0 && time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        const uint8_t* positions = get_classic_positions();
        CRGB color = this->color_ramp.getValue();

        for (int i = 0; i < cyclotron_color_set_size; i++) {
            uint16_t pos = positions[rotation_index] - 1;
            config.leds[(pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = CRGB::Black;
        }

        rotation_index = (rotation_index + (this->config.clockwise ? 1 : 3)) % 4;

        for (int i = 0; i < cyclotron_color_set_size; i++) {
            uint16_t pos = positions[rotation_index] - 1;
            config.leds[(pos + i + config.num_leds - (cyclotron_color_set_size >> 1)) % config.num_leds] = color;
        }
    }
}
bool RotateAnimation::isDone() { return false; }

void FadeAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    this->fade_value = 0;
    this->done = false;
    if (fade_out) fill_solid(config.leds, config.num_leds, this->color_ramp.getValue());
    else fill_solid(config.leds, config.num_leds, CRGB::Black);
    this->time_since_last_update = 0;
}

void FadeAnimation::update(uint32_t dt) {
    Animation::update(dt);
    if (done) return;
    uint16_t step_time_ms = this->speed_ramp.getValue() / 256;
    time_since_last_update += dt;
    if (time_since_last_update >= step_time_ms) {
        time_since_last_update = 0;
        if (fade_value < 255) fade_value += 1;
        else { fade_value = 255; done = true; }
        uint8_t scale = fade_out ? 255 - fade_value : fade_value;
        CRGB color = this->color_ramp.getValue();
        for(int i = 0; i < config.num_leds; i++) {
            config.leds[i] = color;
            config.leds[i].nscale8(scale);
        }
    }
}
bool FadeAnimation::isDone() { return done; }

// --- Feedback Animations ---

void FeedbackRainbowAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
    this->elapsed_ms = 0;

    // Clear the entire target strip so any pixels from previous animations are
    // fully blanked before the rainbow begins. This prevents ghosting when the
    // active LED count changes between feedback requests.
    if (config.leds == g_cyclotron_leds) {
        fill_solid(g_cyclotron_leds, NUM_LEDS_CYCLOTRON, CRGB::Black);
    } else if (config.leds && config.num_leds > 0) {
        fill_solid(config.leds, config.num_leds, CRGB::Black);
    }
}

void FeedbackRainbowAnimation::update(uint32_t dt) {
    Animation::update(dt);
    this->elapsed_ms += dt;

    // Animate a full rainbow across the active LEDs.  The hue shifts over
    // time so the colors appear to rotate while always covering the spectrum.
    uint8_t start_hue = (this->elapsed_ms / 10) & 0xFF;
    uint8_t hue_step = config.num_leds ? (255 / config.num_leds) : 0;
    fill_rainbow(config.leds, config.num_leds, start_hue, hue_step);

    // Ensure any LEDs beyond the active range remain dark to prevent
    // residual pixels from previous animations when fewer LEDs are active.
    if (config.leds == g_cyclotron_leds) {
        for (int i = config.num_leds; i < NUM_LEDS_CYCLOTRON; i++) {
            g_cyclotron_leds[i] = CRGB::Black;
        }
    }

}

bool FeedbackRainbowAnimation::isDone() {
    return this->elapsed_ms >= this->duration_ms;
}

void FeedbackRainbowAnimation::updateConfig(const AnimationConfig& config, uint32_t extend_ms) {
    uint32_t saved_elapsed = this->elapsed_ms;
    Animation::start(config);
    this->elapsed_ms = saved_elapsed;
    this->duration_ms += extend_ms;
}

// --- Party Mode Animations ---

void PartyRainbowFadeAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
}

void PartyRainbowFadeAnimation::update(uint32_t dt) {
    Animation::update(dt);
    CRGB color = CHSV(state->rainbow_hue, 255, 255);
    fill_solid(config.leds, config.num_leds, color);
}

bool PartyRainbowFadeAnimation::isDone() {
    return false;
}


void PartyRandomSparkleAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
}

void PartyRandomSparkleAnimation::update(uint32_t dt) {
    Animation::update(dt);
    fadeToBlackBy(config.leds, config.num_leds, 32);
    if (state->sparkle_strip_index == this->strip_index) {
        if (config.num_leds > 0) {
            config.leds[rand() % config.num_leds] = state->sparkle_color;
        }
        state->sparkle_strip_index = -1;
    }
}

bool PartyRandomSparkleAnimation::isDone() {
    return false;
}

void BeatMeterAnimation::start(const AnimationConfig& config) {
    Animation::start(config);
}

void BeatMeterAnimation::update(uint32_t dt) {
    Animation::update(dt);

    fill_solid(config.leds, config.num_leds, CRGB::Black);
    if (state->beat_meter_max_level > 0) {
        uint8_t threshold = ((state->beat_meter_level + 1) * config.num_leds) / state->beat_meter_max_level;
        CRGB color = this->color_ramp.getValue();
        for (int i = 0; i < threshold; i++) {
            config.leds[i] = color;
        }
    }
}

bool BeatMeterAnimation::isDone() {
    return false;
}
