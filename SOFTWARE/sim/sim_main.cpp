// Animation preview renderer.
//
// Unlike the previous simulator, which re-implemented every effect from
// scratch (and drifted badly from the firmware - the "video_game" renders
// were a solid color because of a name-parsing bug), this driver compiles
// the firmware's own animations.cpp / party_sequences.cpp / RAMP sources
// and only supplies what the hardware would: the LED buffers, the
// controllers, a virtual clock, and the same play/stop sequencing the pack
// state machine performs. What the GIFs show is what the pack does.
//
// Environment (matching .github/workflows/render-animations.yml):
//   ANIMATION_NAME  e.g. movie_cyclotron_sequence, party_rainbow_fade
//   LED_COUNT       pixels to render (cyclotron ring size for ring clips)
//   COLOR           red|green|blue|white|rainbow (party generates its own)
//   LAYOUT          ring|strip
//   DURATION_MS     optional override of the clip length

#include <FastLED.h>
#include "frame_recorder.h"

#include "addressable_LED_support.h"
#include "animation_controller.h"
#include "animations.h"
#include "cyclotron_sequences.h"
#include "future_sequences.h"
#include "pack_state.h"
#include "party_sequences.h"
#include "powercell_sequences.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// --- Firmware globals the hardware build defines elsewhere ---
uint32_t g_sim_millis = 0;
CRGB g_powercell_leds[NUM_LEDS_POWERCELL];
CRGB g_cyclotron_leds[NUM_LEDS_CYCLOTRON];
CRGB g_future_leds[NUM_LEDS_FUTURE];
AnimationController g_powercell_controller;
AnimationController g_cyclotron_controller;
AnimationController g_future_controller;

// Animations quantize their step timing to the update tick (accumulators
// reset to zero on each step), so the simulator advances in the same 4 ms
// ticks as the firmware's repeating timer and records every fourth tick,
// giving ~60 fps output without distorting fine-grained step times.
static const int TICK_MS = 4;
static const int TICKS_PER_FRAME = 4;
static const int FRAME_MS = TICK_MS * TICKS_PER_FRAME;
static const int SEQ_FRAMES = 720;      // ~11.5 s: power-up, idle, shutdown
static const int PARTY_FRAMES = 480;    // ~7.7 s party loop
static Layout g_layout = Layout::Strip;
static int g_frame = 0;

static void record_frame(const CRGB* leds, int n) {
  std::vector<uint8_t> row(n * 3);
  for (int i = 0; i < n; ++i) {
    row[i * 3 + 0] = leds[i].r;
    row[i * 3 + 1] = leds[i].g;
    row[i * 3 + 2] = leds[i].b;
  }
  char name[256];
  std::snprintf(name, sizeof(name), "frames/frame_%05d.ppm", g_frame++);
  std::filesystem::create_directories("frames");
  write_frame_ppm(name, row.data(), n, g_layout);
}

static CRGB parse_color(const std::string& name) {
  if (name == "red") return CRGB::Red;
  if (name == "green") return CRGB::Green;
  if (name == "blue") return CRGB::Blue;
  if (name == "white") return CRGB::White;
  if (name == "orange") return CRGB::Orange;
  return CRGB::White;
}

static AnimationConfig make_config(CRGB* leds, int n, CRGB color,
                                   uint16_t speed) {
  AnimationConfig config;
  config.leds = leds;
  config.num_leds = n;
  config.color = color;
  config.speed = speed;
  return config;
}

static void reap_all() {
  g_powercell_controller.reap();
  g_cyclotron_controller.reap();
  g_future_controller.reap();
}

// One firmware timer tick.
static void step_tick() {
  g_sim_millis += TICK_MS;
  g_powercell_controller.update(TICK_MS);
  g_cyclotron_controller.update(TICK_MS);
  g_future_controller.update(TICK_MS);
  party_mode_run();
  reap_all();
}

static void step_controllers() {
  for (int t = 0; t < TICKS_PER_FRAME; ++t) {
    step_tick();
  }
}

// Timings shared with the firmware's sequences: startup waterfall/fade 4800,
// idle cycle 850 (the ADJ midpoint), shutdown drain/fade 2900.
static const uint16_t STARTUP_MS = 4800;
static const uint16_t IDLE_CYCLE_MS = 850;
static const uint16_t SHUTDOWN_MS = 2900;
static const uint32_t SHUTDOWN_AT_MS = 8500;
static const uint32_t OFF_AT_MS = 10800;

struct Sequencer {
  std::string mode;  // movie | video_game | tvg | afterlife
  CRGB color;
  bool afterlife() const { return mode == "afterlife"; }
  bool snap() const { return mode == "movie"; }
};

static void run_powercell(const Sequencer& seq, int frames) {
  int phase = 0;
  if (seq.snap()) {
    g_powercell_controller.play(
        std::make_unique<ScrollAnimation>(),
        make_config(g_powercell_leds, NUM_LEDS_POWERCELL, seq.color,
                    IDLE_CYCLE_MS));
  } else {
    g_powercell_controller.play(
        std::make_unique<WaterfallAnimation>(),
        make_config(g_powercell_leds, NUM_LEDS_POWERCELL, seq.color,
                    STARTUP_MS));
  }
  for (int i = 0; i < frames; ++i) {
    if (!seq.snap() && phase == 0 && !g_powercell_controller.isRunning()) {
      phase = 1;
      g_powercell_controller.play(
          std::make_unique<ScrollAnimation>(),
          make_config(g_powercell_leds, NUM_LEDS_POWERCELL, seq.color,
                      IDLE_CYCLE_MS));
    }
    if (seq.snap() && g_sim_millis >= OFF_AT_MS && phase == 0) {
      phase = 2;  // classic snap packs cut to black
      g_powercell_controller.stop();
      fill_solid(g_powercell_leds, NUM_LEDS_POWERCELL, CRGB::Black);
    } else if (!seq.snap() && phase == 1 && g_sim_millis >= SHUTDOWN_AT_MS) {
      phase = 2;
      g_powercell_controller.play(
          std::make_unique<DrainAnimation>(),
          make_config(g_powercell_leds, NUM_LEDS_POWERCELL, seq.color,
                      SHUTDOWN_MS));
    }
    step_controllers();
    record_frame(g_powercell_leds, NUM_LEDS_POWERCELL);
  }
}

static void run_cyclotron(const Sequencer& seq, int n, int frames) {
  g_cyclotron_led_count = (uint8_t)n;
  int phase = 0;
  rampUnsignedLong mult(0);

  if (seq.afterlife()) {
    // pack_combo_startup: spin up from 5x to the ring-size target over 6 s;
    // pack_combo_powerdown: fade the color out while the ring slows.
    g_cyclotron_controller.play(
        std::make_unique<CylonAnimation>(),
        make_config(g_cyclotron_leds, n, seq.color, 1000));
    mult.go(5u << 16);
    mult.go(afterlife_target_speed_x() << 16, 6000, QUADRATIC_IN);
  } else if (seq.snap()) {
    g_cyclotron_controller.play(
        std::make_unique<RotateAnimation>(),
        make_config(g_cyclotron_leds, n, seq.color, IDLE_CYCLE_MS));
  } else {  // video_game / tvg fade variants
    g_cyclotron_controller.play(
        std::make_unique<FadeAnimation>(false),
        make_config(g_cyclotron_leds, n, seq.color, STARTUP_MS));
  }

  for (int i = 0; i < frames; ++i) {
    if (seq.afterlife()) {
      if (phase == 0 && g_sim_millis >= SHUTDOWN_AT_MS) {
        phase = 1;
        mult.go(0, SHUTDOWN_MS, QUADRATIC_OUT);
        g_cyclotron_controller.enqueue(std::make_unique<ChangeColorAction>(
            CRGB::Black, SHUTDOWN_MS, QUADRATIC_OUT));
      } else if (phase == 1 && g_sim_millis >= SHUTDOWN_AT_MS + SHUTDOWN_MS) {
        phase = 2;
        g_cyclotron_controller.stop();
        fill_solid(g_cyclotron_leds, n, CRGB::Black);
      }
      for (int t = 0; t < TICKS_PER_FRAME; ++t) {
        uint32_t m = mult.update();
        if (m > 0) {
          if (auto* anim = g_cyclotron_controller.getCurrentAnimation()) {
            anim->setSpeed((1000ull * (1 << 16)) / m, 0);
          }
        }
        step_tick();
      }
      record_frame(g_cyclotron_leds, n);
      continue;
    } else if (seq.snap()) {
      if (phase == 0 && g_sim_millis >= OFF_AT_MS) {
        phase = 1;
        g_cyclotron_controller.stop();
        fill_solid(g_cyclotron_leds, n, CRGB::Black);
      }
    } else {
      if (phase == 0 && !g_cyclotron_controller.isRunning()) {
        phase = 1;
        AnimationConfig config =
            make_config(g_cyclotron_leds, n, seq.color, IDLE_CYCLE_MS);
        config.fade_amount = 4;
        config.steps = 64;
        g_cyclotron_controller.play(std::make_unique<RotateFadeAnimation>(),
                                    config);
      } else if (phase == 1 && g_sim_millis >= SHUTDOWN_AT_MS) {
        phase = 2;
        g_cyclotron_controller.play(
            std::make_unique<FadeAnimation>(true),
            make_config(g_cyclotron_leds, n, seq.color, SHUTDOWN_MS));
      }
    }
    step_controllers();
    record_frame(g_cyclotron_leds, n);
  }
}

static void run_future(const Sequencer& seq, int frames) {
  // The N-Filter strip is the vent light (full_vent() in monitors.cpp):
  // dark, vent pattern for the sequence, dark again.
  int phase = 0;
  for (int i = 0; i < frames; ++i) {
    if (phase == 0 && g_sim_millis >= 500) {
      phase = 1;
      AnimationConfig config =
          make_config(g_future_leds, NUM_LEDS_FUTURE, seq.color, 150);
      if (seq.afterlife()) {
        config.speed = 600;
        config.clockwise = false;
        g_future_controller.play(std::make_unique<ShiftRotateAnimation>(),
                                 config);
      } else {
        g_future_controller.play(std::make_unique<StrobeAnimation>(), config);
      }
    } else if (phase == 1 && g_sim_millis >= OFF_AT_MS) {
      phase = 2;
      g_future_controller.stop();
      fill_solid(g_future_leds, NUM_LEDS_FUTURE, CRGB::Black);
    }
    step_controllers();
    record_frame(g_future_leds, NUM_LEDS_FUTURE);
  }
}

static void run_party(party_animation_t animation, int n, int frames) {
  CRGB* strip = g_cyclotron_leds;
  int strip_len = n;
  if (n == NUM_LEDS_POWERCELL) {
    strip = g_powercell_leds;
  } else if (n == NUM_LEDS_FUTURE) {
    strip = g_future_leds;
  } else {
    g_cyclotron_led_count = (uint8_t)n;
  }
  party_mode_set_animation(animation);
  for (int i = 0; i < frames; ++i) {
    step_controllers();
    record_frame(strip, strip_len);
  }
  party_mode_stop();
}

int main() {
  std::string name = "party_rainbow_fade";
  if (const char* env = std::getenv("ANIMATION_NAME")) name = env;
  int led_count = 16;
  if (const char* env = std::getenv("LED_COUNT")) led_count = std::atoi(env);
  std::string color_name = "white";
  if (const char* env = std::getenv("COLOR")) color_name = env;
  if (const char* env = std::getenv("LAYOUT")) {
    if (std::string(env) == "ring") g_layout = Layout::Ring;
  }

  std::srand(20260829);  // deterministic party colors across renders

  static const struct {
    const char* name;
    party_animation_t animation;
  } kPartyNames[] = {
      {"party_rainbow_fade", PARTY_ANIMATION_RAINBOW_FADE},
      {"party_cylon_scanner", PARTY_ANIMATION_CYLON_SCANNER},
      {"party_random_sparkle", PARTY_ANIMATION_RANDOM_SPARKLE},
      {"party_beat_meter", PARTY_ANIMATION_BEAT_METER},
  };
  static const char* kModes[] = {"movie", "video_game", "tvg", "afterlife"};

  int frames = 0;
  if (const char* env = std::getenv("DURATION_MS")) {
    frames = std::atoi(env) / FRAME_MS;
  }

  for (const auto& party : kPartyNames) {
    if (name == party.name) {
      run_party(party.animation, led_count, frames ? frames : PARTY_FRAMES);
      return 0;
    }
  }

  // "<mode>_<base>" split on the known mode names, not the first underscore:
  // splitting naively turned video_game_* into mode "video", which matched
  // nothing and rendered a static fallback.
  for (const char* mode : kModes) {
    std::string prefix = std::string(mode) + "_";
    if (name.rfind(prefix, 0) != 0) continue;
    std::string base = name.substr(prefix.size());
    Sequencer seq{mode, parse_color(color_name)};
    if (!frames) frames = SEQ_FRAMES;
    if (base == "powercell_sequence") {
      run_powercell(seq, frames);
    } else if (base == "cyclotron_sequence") {
      run_cyclotron(seq, led_count, frames);
    } else if (base == "future_sequence") {
      run_future(seq, frames);
    } else {
      std::fprintf(stderr, "unknown animation base: %s\n", base.c_str());
      return 1;
    }
    return 0;
  }

  std::fprintf(stderr, "unknown animation name: %s\n", name.c_str());
  return 1;
}
