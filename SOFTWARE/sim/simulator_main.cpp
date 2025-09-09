#include "arduino_stubs.h"
#include "fastled_shim.h"
#include "frame_recorder.h"
#include <filesystem>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>

// declare user sketch entry points
void setup();
void loop();
int animation_duration_frames();

static int g_frame = 0;
static Layout g_layout = Layout::Strip;

static void record_frame(const CRGB* leds, int n, uint8_t brightness) {
  std::vector<uint8_t> row(n*3);
  for (int i=0;i<n;++i) {
    row[i*3+0] = (leds[i].r * brightness) / 255;
    row[i*3+1] = (leds[i].g * brightness) / 255;
    row[i*3+2] = (leds[i].b * brightness) / 255;
  }
  char name[256];
  std::snprintf(name, sizeof(name), "frames/frame_%05d.ppm", g_frame++);
  std::filesystem::create_directories("frames");
  write_frame_ppm(name, row.data(), n, g_layout);
}

int main() {
  if (const char* env = std::getenv("LAYOUT")) {
    std::string val(env);
    if (val == "ring") g_layout = Layout::Ring;
  }
  FastLED::g_show_cb = record_frame;
  setup();
  const int target_fps = 60;
  const int frame_ms = 1000/target_fps;
  int total_frames = animation_duration_frames();
  if (const char* dur = std::getenv("DURATION_MS")) {
    total_frames = std::atoi(dur) / frame_ms;
  }
  for (int i = 0; i < total_frames; ++i) {
    loop();
    delay(frame_ms);
  }
  return 0;
}
