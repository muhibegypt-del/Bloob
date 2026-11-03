// Host tests for Headphones mode (3.2): the binaural room responses have the
// cues a head gives (the near ear first and louder, a room after), and the
// chain turns 5.1 into stereo that never clips.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
}

#include "binaural.h"
#include "headphones.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static double energy(const std::vector<float> &v, size_t from = 0, size_t to = 0) {
  double e = 0;
  if (!to || to > v.size()) to = v.size();
  for (size_t i = from; i < to; i++) e += static_cast<double>(v[i]) * v[i];
  return e;
}

static size_t onset(const std::vector<float> &v) {
  float peak = 0;
  for (float x : v) peak = std::max(peak, std::fabs(x));
  for (size_t i = 0; i < v.size(); i++)
    if (std::fabs(v[i]) > 0.2f * peak) return i;
  return v.size();
}

static void test_responses() {
  auto speakers = binaural::layout_speakers("7.1");
  CHECK(speakers.size() == 8 && speakers[3].name == "LFE");
  for (const auto &room : binaural::rooms()) {
    binaural::Responses r = binaural::compute(room, speakers, 48000);
    CHECK(r.channels.size() == 16);
    CHECK(r.length > 48000 / 10 && r.length <= 48000 / 2);
    // LFE has no direction: silent (the filter mixes it).
    CHECK(energy(r.channels[6]) == 0 && energy(r.channels[7]) == 0);
    // Front left (speaker 0): the left ear hears it first and louder.
    CHECK(onset(r.channels[0]) < onset(r.channels[1]));
    CHECK(energy(r.channels[0]) > energy(r.channels[1]) * 1.5);
    // Front right mirrors it.
    CHECK(onset(r.channels[3]) < onset(r.channels[2]));
    // A room: sound keeps arriving after 50 ms, quieter than the start.
    size_t ms50 = 2400;
    double late = energy(r.channels[0], ms50), early = energy(r.channels[0], 0, ms50);
    CHECK(late > 0 && late < early);
    // Front and back differ (pinna and shoulder cues), even for the same side.
    CHECK(std::fabs(energy(r.channels[0]) - energy(r.channels[8])) > 1e-6);
    for (const auto &ch : r.channels)
      for (float x : ch) CHECK(std::isfinite(x));
  }
  CHECK(std::string(binaural::room_named("nope").id) == "lounge");
  CHECK(std::string(binaural::room_named("cinema").name) == "Cinema");
}

static void test_filters() {
  HeadphoneOptions o;
  std::string f = HeadphoneChain::tail_filters(o);
  CHECK(f.find("equalizer=f=1377:t=q:w=0.31:g=2.6") != std::string::npos);
  CHECK(f.find("lowshelf=f=105") != std::string::npos);
  CHECK(f.find("volume=9dB") != std::string::npos);
  CHECK(f.find("alimiter=limit=0.89") != std::string::npos);
  CHECK(f.find("acompressor") == std::string::npos);
  o.night = true;
  o.xm5 = false;
  f = HeadphoneChain::tail_filters(o);
  CHECK(f.find("acompressor") != std::string::npos && f.find("equalizer") == std::string::npos);
  CHECK(o.summary() == "Lounge \xC2\xB7 Night \xC2\xB7 +9 dB");
}

// Pushes `seconds` of noise on one channel of 5.1 (side) at full scale.
static void render(int channel, double seconds, double *left, double *right, float *peak, long long *samples) {
  HeadphoneOptions o;
  o.enabled = true;
  HeadphoneChain chain;
  AVChannelLayout layout = AV_CHANNEL_LAYOUT_5POINT1;
  std::string error;
  bool ok = chain.open(o, &layout, 48000, 8 /* AV_SAMPLE_FMT_FLTP */, &error);
  CHECK(ok);
  if (!ok) {
    std::printf("  chain: %s\n", error.c_str());
    return;
  }
  CHECK(chain.layout() == "5.1(side)");
  AVFrame *in = av_frame_alloc(), *out = av_frame_alloc();
  uint32_t state = 12345;
  int total = static_cast<int>(seconds * 48000), pts = 0;
  *left = *right = 0;
  *peak = 0;
  *samples = 0;
  auto drain = [&] {
    while (chain.pull(out)) {
      *samples += out->nb_samples;
      const float *l = reinterpret_cast<const float *>(out->extended_data[0]);
      const float *r = reinterpret_cast<const float *>(out->extended_data[1]);
      for (int i = 0; i < out->nb_samples; i++) {
        *left += static_cast<double>(l[i]) * l[i];
        *right += static_cast<double>(r[i]) * r[i];
        *peak = std::max(*peak, std::max(std::fabs(l[i]), std::fabs(r[i])));
      }
      av_frame_unref(out);
    }
  };
  while (pts < total) {
    in->format = 8;
    in->sample_rate = 48000;
    in->nb_samples = 1536;
    av_channel_layout_copy(&in->ch_layout, &layout);
    in->pts = pts;
    av_frame_get_buffer(in, 0);
    for (int c = 0; c < 6; c++) {
      float *d = reinterpret_cast<float *>(in->extended_data[c]);
      for (int i = 0; i < 1536; i++) {
        state = state * 1664525u + 1013904223u;
        d[i] = c == channel ? (static_cast<float>(state >> 8) / 8388608.0f - 1.0f) : 0.0f;
      }
    }
    CHECK(chain.push(in));
    av_frame_unref(in);
    pts += 1536;
    drain();
  }
  chain.push(nullptr);
  drain();
  av_frame_free(&in);
  av_frame_free(&out);
}

static void test_chain() {
  double l = 0, r = 0;
  float peak = 0;
  long long n = 0;
  render(0, 2.0, &l, &r, &peak, &n);  // front left
  CHECK(n > 48000 * 3 / 2 && n <= 48000 * 2 + 48000 / 2);
  CHECK(l > r * 1.2);
  CHECK(peak <= 0.9f && peak > 0.05f);  // the limiter holds; the boost is heard
  render(5, 2.0, &l, &r, &peak, &n);  // right surround
  CHECK(r > l * 1.2);
  CHECK(peak <= 0.9f);
}

int run_headphones_tests() {
  failures = 0;
  test_responses();
  test_filters();
  test_chain();
  if (!failures) std::printf("headphones tests passed\n");
  return failures;
}
