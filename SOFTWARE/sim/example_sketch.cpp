#include "fastled_shim.h"
#include <vector>
#include <cstdlib>
#include <string>
#include <cctype>
#include <cmath>

static int NUM_LEDS = 16;
static std::string g_anim = "example";
static std::string g_mode;
static std::string g_base;
static std::vector<CRGB> leds;
static CRGB g_color(255, 255, 255);
static bool g_rainbow = false;
static int g_total_frames = 360;

int animation_duration_frames() { return g_total_frames; }

static void fade_all(uint8_t amount) {
  for (auto &c : leds) {
    c.r = c.r * amount / 255;
    c.g = c.g * amount / 255;
    c.b = c.b * amount / 255;
  }
}

static CRGB parse_color(const std::string& name) {
  if (name == "red") return CRGB(255,0,0);
  if (name == "green") return CRGB(0,255,0);
  if (name == "blue") return CRGB(0,0,255);
  if (name == "yellow") return CRGB(255,255,0);
  if (name == "purple") return CRGB(128,0,128);
  if (name == "cyan") return CRGB(0,255,255);
  if (name == "white") return CRGB(255,255,255);
  return CRGB(255,255,255);
}

static CRGB wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return CRGB(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return CRGB(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return CRGB(pos * 3, 255 - pos * 3, 0);
}

static void render_powercell_sequence(const std::string &mode, int frame) {
  // 6s total: 1s fill, 4s scroll, 1s drain
  for (auto &c : leds) c = CRGB(0,0,0);
  if (frame < 60) {
    int filled = frame * NUM_LEDS / 60;
    for (int i=0;i<filled;++i) leds[i] = g_color;
  } else if (frame < 300) {
    int pos = frame - 60;
    if (mode == "video_game") pos *= 2;
    pos %= NUM_LEDS;
    if (mode == "afterlife") pos = NUM_LEDS - 1 - pos;
    leds[pos] = g_color;
    if (mode == "tvg") leds[(pos+1)%NUM_LEDS] = g_color;
  } else {
    int remain = NUM_LEDS - ((frame - 300) * NUM_LEDS / 60);
    for (int i=0;i<remain;++i) leds[i] = g_color;
  }
}

static void render_cyclotron_sequence(const std::string &mode, int frame) {
  // 6s: 1s fill, 4s rotation, 1s drain
  for (auto &c : leds) c = CRGB(0,0,0);
  if (frame < 60) {
    int lit = frame * NUM_LEDS / 60;
    for (int i=0;i<lit;++i) leds[i] = g_color;
  } else if (frame < 300) {
    int offset = frame - 60;
    int step = NUM_LEDS / 4;
    if (mode == "video_game") offset = -offset;
    if (mode == "afterlife") offset *= 2;
    int base = (offset % NUM_LEDS + NUM_LEDS) % NUM_LEDS;
    if (mode == "tvg") {
      step = NUM_LEDS / 2;
      for (int k=0;k<2;++k) leds[(base + k*step)%NUM_LEDS] = g_color;
    } else {
      for (int k=0;k<4;++k) leds[(base + k*step)%NUM_LEDS] = g_color;
    }
  } else {
    int remain = NUM_LEDS - ((frame - 300) * NUM_LEDS / 60);
    for (int i=0;i<remain;++i) leds[i] = g_color;
  }
}

static void render_future_sequence(const std::string &mode, int frame) {
  // 6s: 1s fade in, 4s rotate, 1s fade out
  if (frame < 60) {
    uint8_t level = frame * 255 / 60;
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = CRGB(g_color.r*level/255, g_color.g*level/255, g_color.b*level/255);
  } else if (frame < 300) {
    for (auto &c : leds) c = CRGB(0,0,0);
    int pos = frame - 60;
    if (mode == "video_game") pos *= 2;
    if (mode == "afterlife") pos = -pos;
    pos = (pos % NUM_LEDS + NUM_LEDS) % NUM_LEDS;
    leds[pos] = g_color;
    if (mode == "tvg") leds[(pos + NUM_LEDS/2)%NUM_LEDS] = g_color;
  } else {
    uint8_t level = 255 - ((frame - 300) * 255 / 60);
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = CRGB(g_color.r*level/255, g_color.g*level/255, g_color.b*level/255);
  }
}

void setup() {
  if (const char* n = std::getenv("LED_COUNT")) {
    NUM_LEDS = std::atoi(n);
  }
  if (const char* a = std::getenv("ANIMATION_NAME")) {
    g_anim = a;
    auto pos = g_anim.find('_');
    if (pos != std::string::npos && g_anim.rfind("party_",0) != 0) {
      g_mode = g_anim.substr(0,pos);
      g_base = g_anim.substr(pos+1);
    } else {
      g_base = g_anim;
    }
  } else {
    g_base = g_anim;
  }
  if (const char* c = std::getenv("COLOR")) {
    std::string cs(c);
    for (char& ch : cs) ch = std::tolower(static_cast<unsigned char>(ch));
    if (cs == "rainbow" || cs == "multicolor") {
      g_rainbow = true;
    } else {
      g_color = parse_color(cs);
    }
  }
  leds.resize(NUM_LEDS);
  FastLED::addLeds(leds.data(), NUM_LEDS);
  FastLED::setBrightness(200);

  if (g_base == "powercell_sequence" ||
      g_base == "cyclotron_sequence" ||
      g_base == "future_sequence") {
    g_total_frames = 360; // 6s startup+idle+shutdown
  } else if (g_base.rfind("party_", 0) == 0) {
    g_total_frames = 480; // 8s preview for party loops
  } else {
    g_total_frames = 300; // 5s sample for other animations
  }
}

void loop() {
  static int frame = 0;

  if (g_base == "party_rainbow_fade") {
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = wheel((i*256/NUM_LEDS + frame*4) & 0xFF);

  } else if (g_base == "party_cylon_scanner") {
    fade_all(200);
    int pos = frame % (NUM_LEDS*2);
    if (pos >= NUM_LEDS) pos = 2*NUM_LEDS - pos - 1;
    leds[pos] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_base == "party_random_sparkle") {
    fade_all(180);
    int idx = std::rand() % NUM_LEDS;
    leds[idx] = g_rainbow ? wheel(std::rand() & 0xFF) : g_color;

  } else if (g_base == "party_beat_meter") {
    for (auto &c : leds) c = CRGB(0,0,0);
    double s = (std::sin(frame/10.0)+1.0)/2.0;
    int level = static_cast<int>(s * NUM_LEDS);
    for (int i=0;i<level;++i)
      leds[i] = g_rainbow ? wheel((i*256/NUM_LEDS + frame*8)&0xFF) : g_color;

  } else if (g_anim == "shift_rotate") {
    for (int i=NUM_LEDS-1;i>0;--i) leds[i]=leds[i-1];
    leds[0] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "rotate_fade") {
    fade_all(220);
    for (int i=NUM_LEDS-1;i>0;--i) leds[i]=leds[i-1];
    leds[0] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "slime") {
    fade_all(200);
    int drop = frame % NUM_LEDS;
    leds[drop] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "cylon_fade_out") {
    fade_all(200);
    int pos = frame % (NUM_LEDS*2);
    if (pos >= NUM_LEDS) pos = 2*NUM_LEDS - pos -1;
    leds[pos] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "scroll") {
    for (auto &c : leds) c = CRGB(0,0,0);
    leds[frame % NUM_LEDS] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "fill") {
    int filled = frame % (NUM_LEDS+1);
    for (int i=0;i<filled;++i)
      leds[i] = g_rainbow ? wheel((i*256/NUM_LEDS)&0xFF) : g_color;

  } else if (g_anim == "drain") {
    int remain = NUM_LEDS - (frame % (NUM_LEDS+1));
    for (int i=0;i<remain;++i)
      leds[i] = g_rainbow ? wheel((i*256/NUM_LEDS)&0xFF) : g_color;

  } else if (g_anim == "strobe") {
    bool on = ((frame/4) % 2) == 0;
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = on ? (g_rainbow ? wheel((i*256/NUM_LEDS + frame*8)&0xFF) : g_color) : CRGB(0,0,0);

  } else if (g_anim == "waterfall") {
    fade_all(200);
    for (int i=NUM_LEDS-1;i>0;--i) leds[i]=leds[i-1];
    leds[0] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "cylon") {
    for (auto &c : leds) c = CRGB(0,0,0);
    int pos = frame % (NUM_LEDS*2);
    if (pos >= NUM_LEDS) pos = 2*NUM_LEDS - pos -1;
    leds[pos] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "rotate") {
    for (int i=NUM_LEDS-1;i>0;--i) leds[i]=leds[i-1];
    leds[0] = g_rainbow ? wheel((frame*8)&0xFF) : g_color;

  } else if (g_anim == "fade") {
    double s = (std::sin(frame/20.0)+1.0)/2.0;
    CRGB base = g_rainbow ? wheel((frame*8)&0xFF) : g_color;
    for (int i=0;i<NUM_LEDS;++i) {
      leds[i] = CRGB(base.r*s, base.g*s, base.b*s);
    }

  } else if (g_base == "powercell_sequence") {
    render_powercell_sequence(g_mode, frame);

  } else if (g_base == "cyclotron_sequence") {
    render_cyclotron_sequence(g_mode, frame);

  } else if (g_base == "future_sequence") {
    render_future_sequence(g_mode, frame);

  } else if (g_anim == "party_powercell") {
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = wheel((i*256/NUM_LEDS + frame*4) & 0xFF);

  } else if (g_anim == "party_cyclotron") {
    int offset = frame % NUM_LEDS;
    for (int k=0;k<NUM_LEDS;++k)
      leds[k] = CRGB(0,0,0);
    for (int q=0;q<4;++q)
      leds[(offset + q*(NUM_LEDS/4)) % NUM_LEDS] = wheel(((frame*8)+q*32)&0xFF);

  } else if (g_anim == "party_future") {
    for (int i=0;i<NUM_LEDS;++i)
      leds[i] = wheel((i*256/NUM_LEDS + frame*8) & 0xFF);

  } else {
    // fallback: solid color or rainbow
    for (int i=0;i<NUM_LEDS;++i) {
      leds[i] = g_rainbow ? wheel((i*256/NUM_LEDS + frame*8) & 0xFF) : g_color;
    }
  }

  FastLED::show();
  ++frame;
}

