#pragma once
// LG's calibration tables (CALIBRATION-PLAN.md §2.2), as bscpylgtv writes them:
//   1D LUT: 3 × 1024 uint16, planar (all red, then green, then blue), 0–32767.
//   3D LUT: 33³ nodes × 3 uint16, blue slowest and red fastest, each node R G B, 0–4095.
// In memory both hold values in [0, 1]; encode() and decode() convert.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "colour.h"

namespace cal {

struct Lut1D {
  static constexpr int kSize = 1024;
  std::array<std::vector<double>, 3> ch;

  static Lut1D unity();
  // Channel c at input v in [0, 1], linear between entries.
  double eval(int c, double v) const;
  std::vector<uint16_t> encode() const;
  static Lut1D decode(const std::vector<uint16_t> &data);
};

struct Lut3D {
  int n = 33;
  std::vector<Vec3> node;  // index (b * n + g) * n + r

  static Lut3D unity(int n = 33);
  Vec3 &at(int r, int g, int b) { return node[(b * n + g) * n + r]; }
  const Vec3 &at(int r, int g, int b) const { return node[(b * n + g) * n + r]; }
  // Tetrahedral interpolation, the usual way video LUTs are applied.
  Vec3 eval(Vec3 rgb) const;
  std::vector<uint16_t> encode() const;
  static Lut3D decode(const std::vector<uint16_t> &data, int n = 33);
};

// FNV-1a over the little-endian bytes, for comparing tables with bscpylgtv's.
uint64_t fnv1a(const std::vector<uint16_t> &data);
// The base64 the TV's `data` field carries (little-endian bytes).
std::string base64(const std::vector<uint16_t> &data);
// For people and other tools: Resolve/IRIDAS .cube text.
std::string to_cube(const Lut3D &lut, const std::string &title);

}  // namespace cal
