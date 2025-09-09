#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <cmath>

enum class Layout { Strip, Ring };
constexpr double PI = 3.14159265358979323846;

inline bool write_strip_ppm(const std::string& path,
                            const uint8_t* rgb,
                            int nleds,
                            int scale=24) {
  const int w = nleds * scale;
  const int h = scale;
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fprintf(f, "P6\n%d %d\n255\n", w, h);
  for (int y=0; y<h; ++y) {
    for (int x=0; x<w; ++x) {
      int i = x / scale;
      const uint8_t* p = rgb + i*3;
      std::fputc(p[0], f);
      std::fputc(p[1], f);
      std::fputc(p[2], f);
    }
  }
  std::fclose(f);
  return true;
}

inline bool write_ring_ppm(const std::string& path,
                           const uint8_t* rgb,
                           int nleds,
                           int scale=16) {
  const int w = 16 * scale;
  const int h = 16 * scale;
  std::vector<uint8_t> img(w*h*3, 0);
  const double radius = (std::min(w, h) - scale*2) / 2.0;
  const double cx = w / 2.0;
  const double cy = h / 2.0;
  for (int i=0; i<nleds; ++i) {
    double ang = 2.0 * PI * i / nleds;
    double ledx = cx + radius * std::cos(ang);
    double ledy = cy + radius * std::sin(ang);
    for (int dy=0; dy<scale; ++dy) {
      for (int dx=0; dx<scale; ++dx) {
        int x = static_cast<int>(ledx) + dx - scale/2;
        int y = static_cast<int>(ledy) + dy - scale/2;
        if (x>=0 && x<w && y>=0 && y<h) {
          size_t idx = (y*w + x) * 3;
          const uint8_t* p = rgb + i*3;
          img[idx+0] = p[0];
          img[idx+1] = p[1];
          img[idx+2] = p[2];
        }
      }
    }
  }
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fprintf(f, "P6\n%d %d\n255\n", w, h);
  std::fwrite(img.data(), 1, img.size(), f);
  std::fclose(f);
  return true;
}

inline bool write_frame_ppm(const std::string& path,
                            const uint8_t* rgb,
                            int nleds,
                            Layout layout) {
  if (layout == Layout::Ring) {
    return write_ring_ppm(path, rgb, nleds);
  }
  return write_strip_ppm(path, rgb, nleds);
}

