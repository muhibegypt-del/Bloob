// Bloob's binaural room responses (Headphones mode, 3.2). See binaural.h.
#include "binaural.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace binaural {

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kSpeed = 343.0f;      // speed of sound, m/s
constexpr float kHeadRadius = 0.0875f;  // Brown and Duda's average head, m
constexpr float kEarHeight = 1.2f;    // listener's ears above the floor, m
constexpr int kScratch = 192;         // samples for one arrival through the head
constexpr int kPad = 4;               // scratch samples before time zero
constexpr float kScale = 0.45f;       // overall level: headroom for many speakers at once

float rad(float deg) { return deg * kPi / 180.0f; }

struct Vec {
  float x, y, z;  // x to the left, y ahead, z up (metres, listener's head at 0)
};
float length(Vec v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

// Adds `value` at a fractional position (linear split between two samples).
void add_at(std::vector<float> &buf, float pos, float value) {
  if (pos < 0) return;
  size_t i = static_cast<size_t>(pos);
  float f = pos - static_cast<float>(i);
  if (i < buf.size()) buf[i] += value * (1 - f);
  if (i + 1 < buf.size()) buf[i + 1] += value * f;
}

// One arrival at one ear through Brown and Duda's head: interaural delay,
// pinna echoes, shoulder echo, then the head-shadow filter. `dir` is a unit
// vector towards the source; `left` picks the ear; `offset` (samples, < 1)
// carries the propagation delay's fraction. Time zero is scratch[kPad].
void ear_arrival(std::vector<float> &scratch, Vec dir, bool left, float offset, int rate) {
  std::fill(scratch.begin(), scratch.end(), 0.0f);
  // Angle between the source and this ear's axis (0: straight into the ear).
  float side = left ? dir.x : -dir.x;
  float incidence = std::acos(std::max(-1.0f, std::min(1.0f, side)));
  // Arrival time against the head's centre, shifted so it is never negative.
  float a_c = kHeadRadius / kSpeed;
  float itd = incidence < kPi / 2 ? -a_c * std::cos(incidence) : a_c * (incidence - kPi / 2);
  float start = kPad + offset + (itd + a_c) * rate;
  // Pinna echoes (Brown and Duda's table, delays in samples at 44.1 kHz),
  // softened to half strength so they cue direction without colouring much.
  float azimuth = std::fabs(std::atan2(dir.x, dir.y)) * 180.0f / kPi;  // 0 ahead .. 180 behind
  float elevation = std::asin(std::max(-1.0f, std::min(1.0f, dir.z))) * 180.0f / kPi;
  static const float rho[] = {0.5f, -1.0f, 0.5f, -0.25f, 0.25f};
  static const float A[] = {1, 5, 5, 5, 5}, B[] = {2, 4, 7, 11, 13}, D[] = {1.0f, 0.5f, 0.5f, 0.5f, 0.5f};
  std::vector<float> &s = scratch;
  add_at(s, start, 1.0f);
  for (int k = 0; k < 5; k++) {
    float tau = A[k] * std::cos(rad(azimuth / 2)) * std::sin(rad(D[k] * (90 - elevation))) + B[k];
    add_at(s, start + tau * rate / 44100.0f, 0.5f * rho[k]);
  }
  // Shoulder and torso: one echo, later for sources behind.
  float shoulder_ms = 1.2f * (180 - azimuth) / 180 *
                      (1 - 0.00004f * std::pow((elevation - 80) * 180 / (180 + azimuth), 2.0f));
  if (shoulder_ms > 0.05f) add_at(s, start + shoulder_ms * rate / 1000, 0.3f);
  // Head shadow: a one-pole, one-zero filter whose high frequencies rise for
  // the near ear (alpha up to 2) and fall up to 20 dB for the far ear.
  float theta = incidence * 180 / kPi;
  float alpha = 1.05f + 0.95f * std::cos(theta / 150.0f * kPi);
  float K = 2.0f * rate * kHeadRadius / (2 * kSpeed);
  float b0 = (1 + alpha * K) / (1 + K), b1 = (1 - alpha * K) / (1 + K), a1 = (1 - K) / (1 + K);
  float x1 = 0, y1 = 0;
  for (auto &v : s) {
    float y = b0 * v + b1 * x1 - a1 * y1;
    x1 = v;
    y1 = y;
    v = y;
  }
}

// One-pole low-pass, in place: each wall bounce darkens the sound.
void darken(std::vector<float> &buf, int bounces, int rate) {
  if (bounces <= 0) return;
  float c = std::exp(-2 * kPi * 7000.0f / rate);
  for (int b = 0; b < bounces; b++) {
    float y = 0;
    for (auto &v : buf) {
      y = (1 - c) * v + c * y;
      v = y;
    }
  }
}

// A small, repeatable noise source (the same room every time).
struct Noise {
  uint32_t state;
  explicit Noise(uint32_t seed) : state(seed * 2654435761u + 1) {}
  float next() {
    float sum = 0;
    for (int i = 0; i < 4; i++) {
      state ^= state << 13;
      state ^= state >> 17;
      state ^= state << 5;
      sum += static_cast<float>(state & 0xffff) / 65535.0f - 0.5f;
    }
    return sum;  // roughly Gaussian, variance 1/3
  }
};

}  // namespace

const std::vector<Room> &rooms() {
  static const std::vector<Room> list = {
      // id, name, width, depth, height, distance, reflection, tail dB, RT60
      {"lounge", "Lounge", 5.5f, 7.0f, 2.8f, 2.4f, 0.65f, -13.0f, 0.36f},
      {"studio", "Studio", 4.5f, 5.5f, 2.8f, 1.8f, 0.55f, -17.0f, 0.24f},
      {"cinema", "Cinema", 12.0f, 16.0f, 6.0f, 5.5f, 0.7f, -10.0f, 0.55f},
  };
  return list;
}

const Room &room_named(const std::string &id) {
  for (const auto &r : rooms())
    if (id == r.id) return r;
  return rooms().front();
}

std::vector<Speaker> layout_speakers(const std::string &layout) {
  // ITU-R BS.775 / Dolby placements: fronts at 30, sides at 100 (110 in 5.1), backs at 145.
  if (layout == "7.1")
    return {{"FL", 30, 0, 0}, {"FR", -30, 0, 0}, {"FC", 0, 0, 0}, {"LFE", 0, 0, 0},
            {"BL", 145, 0, 0}, {"BR", -145, 0, 0}, {"SL", 100, 0, 0}, {"SR", -100, 0, 0}};
  if (layout == "5.1(side)")
    return {{"FL", 30, 0, 0}, {"FR", -30, 0, 0}, {"FC", 0, 0, 0}, {"LFE", 0, 0, 0}, {"SL", 110, 0, 0}, {"SR", -110, 0, 0}};
  return {{"FL", 30, 0, 0}, {"FR", -30, 0, 0}};
}

Responses compute(const Room &room, const std::vector<Speaker> &speakers, int rate) {
  Responses out;
  out.rate = rate;
  float seconds = std::min(0.7f * room.rt60 + 0.03f, 0.42f);
  out.length = static_cast<int>(seconds * rate);
  out.channels.assign(speakers.size() * 2, std::vector<float>(static_cast<size_t>(out.length), 0.0f));
  const float d = room.distance;
  // The room around the listener: a little more space ahead than behind.
  const float lo[3] = {-room.width / 2, -0.45f * room.depth, -kEarHeight};
  const float hi[3] = {room.width / 2, 0.55f * room.depth, room.height - kEarHeight};
  std::vector<float> scratch(kScratch);
  const size_t len = static_cast<size_t>(out.length);
  // Direct sound arrives a few samples in, the same for every speaker.
  const float direct_at = 8.0f;

  for (size_t k = 0; k < speakers.size(); k++) {
    const Speaker &sp = speakers[k];
    if (sp.name == "LFE") continue;
    float gain = kScale * std::pow(10.0f, sp.gain_db / 20);
    Vec src = {d * std::sin(rad(sp.azimuth)) * std::cos(rad(sp.elevation)), d * std::cos(rad(sp.azimuth)) * std::cos(rad(sp.elevation)),
               d * std::sin(rad(sp.elevation))};
    float s[3] = {src.x, src.y, src.z};
    for (int a = 0; a < 3; a++) s[a] = std::max(lo[a] + 0.2f, std::min(hi[a] - 0.2f, s[a]));
    double direct_energy = 0;
    // Image sources: along each axis, mirror positions 2kL +/- s with |2k| or
    // |2k-1| bounces; keep every combination of three bounces or fewer.
    for (int ix = -2; ix <= 2; ix++)
      for (int sx = 0; sx < 2; sx++)
        for (int iy = -2; iy <= 2; iy++)
          for (int sy = 0; sy < 2; sy++)
            for (int iz = -2; iz <= 2; iz++)
              for (int sz = 0; sz < 2; sz++) {
                int idx[3] = {ix, iy, iz}, sign[3] = {sx, sy, sz};
                float p[3];
                int bounces = 0;
                for (int a = 0; a < 3; a++) {
                  float L = hi[a] - lo[a], local = s[a] - lo[a];
                  p[a] = lo[a] + 2 * idx[a] * L + (sign[a] ? -local : local);
                  bounces += sign[a] ? std::abs(2 * idx[a] - 1) : std::abs(2 * idx[a]);
                }
                if (bounces > 3) continue;
                Vec v = {p[0], p[1], p[2]};
                float r = length(v);
                if (r < 0.1f) continue;
                Vec dir = {v.x / r, v.y / r, v.z / r};
                float delay = direct_at + (r - d) / kSpeed * rate;
                if (delay + kScratch >= len) continue;
                float amp = gain * std::pow(room.reflection, static_cast<float>(bounces)) * d / r;
                int whole = static_cast<int>(delay);
                for (int ear = 0; ear < 2; ear++) {
                  ear_arrival(scratch, dir, ear == 0, delay - whole, rate);
                  darken(scratch, bounces, rate);
                  std::vector<float> &ch = out.channels[2 * k + static_cast<size_t>(ear)];
                  for (int i = 0; i < kScratch; i++) {
                    int at = whole - kPad + i;
                    if (at >= 0 && static_cast<size_t>(at) < len) ch[static_cast<size_t>(at)] += amp * scratch[static_cast<size_t>(i)];
                  }
                  if (bounces == 0)
                    for (float v2 : scratch) direct_energy += static_cast<double>(amp * v2) * (amp * v2) / 2;
                }
              }

    // The diffuse tail: low frequencies shared by both ears (as in a real
    // room), highs different at each; highs die away faster than lows.
    float tail_rms = static_cast<float>(std::sqrt(direct_energy * std::pow(10.0, room.tail_db / 10.0)));
    Noise common(static_cast<uint32_t>(k * 7 + 1));
    Noise own[2] = {Noise(static_cast<uint32_t>(k * 7 + 2)), Noise(static_cast<uint32_t>(k * 7 + 3))};
    float c500 = std::exp(-2 * kPi * 500.0f / rate), c3k = std::exp(-2 * kPi * 3000.0f / rate);
    float onset = direct_at + 0.004f * rate, fade = 0.015f * rate;
    for (int ear = 0; ear < 2; ear++) {
      std::vector<float> tail(len, 0.0f);
      float lc = 0, lo_own = 0, low = 0;
      double energy = 0;
      Noise shared = common;
      for (size_t i = 0; i < len; i++) {
        float cn = shared.next(), on = own[ear].next();
        lc = (1 - c500) * cn + c500 * lc;
        lo_own = (1 - c500) * on + c500 * lo_own;
        float n = lc + (on - lo_own);
        low = (1 - c3k) * n + c3k * low;
        float t = (static_cast<float>(i) - onset) / rate;
        if (t < 0) continue;
        float rise = static_cast<float>(i) - onset < fade ? 0.5f - 0.5f * std::cos(kPi * (static_cast<float>(i) - onset) / fade) : 1.0f;
        float env_low = std::exp(-6.91f * t / room.rt60), env_high = std::exp(-6.91f * t / (0.55f * room.rt60));
        tail[i] = rise * (low * env_low + (n - low) * env_high);
        energy += static_cast<double>(tail[i]) * tail[i];
      }
      if (energy <= 0) continue;
      float scale = tail_rms / static_cast<float>(std::sqrt(energy));
      // A gentle end, so the cut is never heard.
      size_t end_fade = std::min(len, static_cast<size_t>(0.03f * rate));
      std::vector<float> &ch = out.channels[2 * k + static_cast<size_t>(ear)];
      for (size_t i = 0; i < len; i++) {
        float g = i + end_fade >= len ? static_cast<float>(len - i) / end_fade : 1.0f;
        ch[i] += tail[i] * scale;
        ch[i] *= g;
      }
    }
  }
  return out;
}

}  // namespace binaural
