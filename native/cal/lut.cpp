#include "lut.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cal {

Lut1D Lut1D::unity() {
  Lut1D l;
  for (auto &c : l.ch) {
    c.resize(kSize);
    for (int i = 0; i < kSize; i++) c[i] = static_cast<double>(i) / (kSize - 1);
  }
  return l;
}

double Lut1D::eval(int c, double v) const {
  const auto &t = ch[c];
  double x = std::clamp(v, 0.0, 1.0) * (kSize - 1);
  int i = std::min(static_cast<int>(x), kSize - 2);
  double f = x - i;
  return t[i] + (t[i + 1] - t[i]) * f;
}

std::vector<uint16_t> Lut1D::encode() const {
  std::vector<uint16_t> out;
  out.reserve(3 * kSize);
  for (const auto &c : ch)
    for (double v : c) out.push_back(static_cast<uint16_t>(std::lround(std::clamp(v, 0.0, 1.0) * 32767)));
  return out;
}

Lut1D Lut1D::decode(const std::vector<uint16_t> &data) {
  Lut1D l;
  for (int c = 0; c < 3; c++) {
    l.ch[c].resize(kSize);
    for (int i = 0; i < kSize; i++) {
      size_t k = static_cast<size_t>(c) * kSize + i;
      l.ch[c][i] = k < data.size() ? data[k] / 32767.0 : static_cast<double>(i) / (kSize - 1);
    }
  }
  return l;
}

Lut3D Lut3D::unity(int n) {
  Lut3D l;
  l.n = n;
  l.node.resize(static_cast<size_t>(n) * n * n);
  for (int b = 0; b < n; b++)
    for (int g = 0; g < n; g++)
      for (int r = 0; r < n; r++)
        l.at(r, g, b) = {static_cast<double>(r) / (n - 1), static_cast<double>(g) / (n - 1), static_cast<double>(b) / (n - 1)};
  return l;
}

Vec3 Lut3D::eval(Vec3 in) const {
  double s = n - 1;
  double x = std::clamp(in.x, 0.0, 1.0) * s, y = std::clamp(in.y, 0.0, 1.0) * s, z = std::clamp(in.z, 0.0, 1.0) * s;
  int r = std::min(static_cast<int>(x), n - 2), g = std::min(static_cast<int>(y), n - 2), b = std::min(static_cast<int>(z), n - 2);
  double fr = x - r, fg = y - g, fb = z - b;
  const Vec3 &c000 = at(r, g, b), &c111 = at(r + 1, g + 1, b + 1);
  if (fr >= fg && fg >= fb)
    return c000 + (at(r + 1, g, b) - c000) * fr + (at(r + 1, g + 1, b) - at(r + 1, g, b)) * fg + (c111 - at(r + 1, g + 1, b)) * fb;
  if (fr >= fb && fb >= fg)
    return c000 + (at(r + 1, g, b) - c000) * fr + (at(r + 1, g, b + 1) - at(r + 1, g, b)) * fb + (c111 - at(r + 1, g, b + 1)) * fg;
  if (fb >= fr && fr >= fg)
    return c000 + (at(r, g, b + 1) - c000) * fb + (at(r + 1, g, b + 1) - at(r, g, b + 1)) * fr + (c111 - at(r + 1, g, b + 1)) * fg;
  if (fg >= fr && fr >= fb)
    return c000 + (at(r, g + 1, b) - c000) * fg + (at(r + 1, g + 1, b) - at(r, g + 1, b)) * fr + (c111 - at(r + 1, g + 1, b)) * fb;
  if (fg >= fb && fb >= fr)
    return c000 + (at(r, g + 1, b) - c000) * fg + (at(r, g + 1, b + 1) - at(r, g + 1, b)) * fb + (c111 - at(r, g + 1, b + 1)) * fr;
  return c000 + (at(r, g, b + 1) - c000) * fb + (at(r, g + 1, b + 1) - at(r, g, b + 1)) * fg + (c111 - at(r, g + 1, b + 1)) * fr;
}

// 12-bit codes: node value v is v × 4096, so the unity table's top node (4096) is clipped
// to 4095 as bscpylgtv does; 4095 reads back as 1.
static uint16_t code12(double v) { return static_cast<uint16_t>(std::min(4095L, std::lround(std::clamp(v, 0.0, 1.0) * 4096))); }
static double value12(uint16_t c) { return c >= 4095 ? 1.0 : c / 4096.0; }

std::vector<uint16_t> Lut3D::encode() const {
  std::vector<uint16_t> out;
  out.reserve(node.size() * 3);
  for (const Vec3 &v : node) {
    out.push_back(code12(v.x));
    out.push_back(code12(v.y));
    out.push_back(code12(v.z));
  }
  return out;
}

Lut3D Lut3D::decode(const std::vector<uint16_t> &data, int n) {
  Lut3D l = unity(n);
  for (size_t i = 0; i < l.node.size() && 3 * i + 2 < data.size(); i++)
    l.node[i] = {value12(data[3 * i]), value12(data[3 * i + 1]), value12(data[3 * i + 2])};
  return l;
}

uint64_t fnv1a(const std::vector<uint16_t> &data) {
  uint64_t h = 0xcbf29ce484222325ULL;
  for (uint16_t v : data)
    for (int k = 0; k < 2; k++) {
      h ^= static_cast<uint8_t>(v >> (8 * k));
      h *= 0x100000001b3ULL;
    }
  return h;
}

std::string base64(const std::vector<uint16_t> &data) {
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::vector<uint8_t> bytes;
  bytes.reserve(data.size() * 2);
  for (uint16_t v : data) {
    bytes.push_back(static_cast<uint8_t>(v & 0xff));
    bytes.push_back(static_cast<uint8_t>(v >> 8));
  }
  std::string out;
  out.reserve((bytes.size() + 2) / 3 * 4);
  for (size_t i = 0; i < bytes.size(); i += 3) {
    uint32_t n = static_cast<uint32_t>(bytes[i]) << 16;
    if (i + 1 < bytes.size()) n |= static_cast<uint32_t>(bytes[i + 1]) << 8;
    if (i + 2 < bytes.size()) n |= bytes[i + 2];
    out += t[(n >> 18) & 63];
    out += t[(n >> 12) & 63];
    out += i + 1 < bytes.size() ? t[(n >> 6) & 63] : '=';
    out += i + 2 < bytes.size() ? t[n & 63] : '=';
  }
  return out;
}

std::string to_cube(const Lut3D &lut, const std::string &title) {
  std::string out = "TITLE \"" + title + "\"\nLUT_3D_SIZE " + std::to_string(lut.n) + "\n";
  char line[96];
  for (const Vec3 &v : lut.node) {
    std::snprintf(line, sizeof line, "%.6f %.6f %.6f\n", v.x, v.y, v.z);
    out += line;
  }
  return out;
}

}  // namespace cal
