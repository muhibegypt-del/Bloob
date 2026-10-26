// Headphones mode's filter chain (3.2). See headphones.h.
#include "headphones.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libavutil/samplefmt.h>
}

#include "binaural.h"
#include "log.h"

namespace {

std::string num(double v) {
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.4g", v);
  return buf;
}

// Sony WF-1000XM5 to the Harman in-ear target: AutoEQ's parametric result for
// oratory1990's measurement (github.com/jaakkopasanen/AutoEq, MIT).
struct Band {
  const char *type;  // "lowshelf", "highshelf" or "equalizer"
  double f, gain, q;
};
const double kXm5Preamp = -2.6;
const Band kXm5[] = {
    {"lowshelf", 105, 5.4, 0.70},  {"equalizer", 68, -4.8, 0.27},   {"equalizer", 1377, 2.6, 0.31},
    {"equalizer", 1806, -3.5, 1.16}, {"equalizer", 6875, 1.8, 3.49}, {"highshelf", 10000, 2.3, 0.70},
    {"equalizer", 3757, 0.3, 2.56},  {"equalizer", 5057, -0.7, 5.70}, {"equalizer", 449, 0.3, 4.15},
    {"equalizer", 677, -0.2, 2.75},
};

// A little-endian RIFF WAVE file: PCM 16/24/32-bit or 32-bit float.
bool read_wav(const std::string &path, int *rate, int *channels, std::vector<float> *samples, std::string *error) {
  FILE *f = std::fopen(path.c_str(), "rb");
  if (!f) {
    *error = "cannot open " + path;
    return false;
  }
  std::vector<uint8_t> b;
  uint8_t chunk[65536];
  size_t n;
  while ((n = std::fread(chunk, 1, sizeof chunk, f)) > 0 && b.size() < (64u << 20)) b.insert(b.end(), chunk, chunk + n);
  std::fclose(f);
  auto u16 = [&](size_t at) { return static_cast<unsigned>(b[at] | b[at + 1] << 8); };
  auto u32 = [&](size_t at) { return static_cast<uint32_t>(b[at] | b[at + 1] << 8 | b[at + 2] << 16 | static_cast<uint32_t>(b[at + 3]) << 24); };
  if (b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) != 0 || std::memcmp(b.data() + 8, "WAVE", 4) != 0) {
    *error = "not a WAV file";
    return false;
  }
  unsigned format = 0, bits = 0;
  size_t pos = 12, data = 0, data_len = 0;
  while (pos + 8 <= b.size()) {
    uint32_t len = u32(pos + 4);
    if (std::memcmp(b.data() + pos, "fmt ", 4) == 0 && pos + 24 <= b.size()) {
      format = u16(pos + 8);
      *channels = static_cast<int>(u16(pos + 10));
      *rate = static_cast<int>(u32(pos + 12));
      bits = u16(pos + 22);
      if (format == 0xFFFE && pos + 34 <= b.size()) format = u16(pos + 32);  // WAVE_FORMAT_EXTENSIBLE: the sub-format
    } else if (std::memcmp(b.data() + pos, "data", 4) == 0) {
      data = pos + 8;
      data_len = std::min<size_t>(len, b.size() - data);
      break;
    }
    pos += 8 + len + (len & 1);
  }
  if (!data || *channels <= 0 || !(format == 1 || format == 3)) {
    *error = "unsupported WAV (PCM or float only)";
    return false;
  }
  size_t width = bits / 8, frames = data_len / (width * static_cast<size_t>(*channels));
  samples->resize(frames * static_cast<size_t>(*channels));
  for (size_t i = 0; i < samples->size(); i++) {
    const uint8_t *p = b.data() + data + i * width;
    float v = 0;
    if (format == 3 && bits == 32) std::memcpy(&v, p, 4);
    else if (bits == 16) v = static_cast<int16_t>(p[0] | p[1] << 8) / 32768.0f;
    else if (bits == 24) v = static_cast<float>(static_cast<int32_t>(static_cast<uint32_t>(p[0] << 8 | p[1] << 16 | p[2] << 24)) >> 8) / 8388608.0f;
    else if (bits == 32) v = static_cast<float>(static_cast<int32_t>(u32(data + i * width))) / 2147483648.0f;
    (*samples)[i] = v;
  }
  return true;
}

// Interleaved responses (pairs per speaker, in `speakers` order) from a
// measured file. HeSuVi's 14 channels: FL-l FL-r SL-l SL-r BL-l BL-r FC-l
// FR-r FR-l SR-r SR-l BR-r BR-l FC-r. Bloob's 16: FL FR FC LFE BL BR SL SR pairs.
bool measured(const std::string &path, const std::vector<binaural::Speaker> &speakers, int *frames,
              std::vector<float> *out, std::string *error) {
  int rate = 0, ch = 0;
  std::vector<float> s;
  if (!read_wav(path, &rate, &ch, &s, error)) return false;
  if (rate != 48000) {
    *error = "the response must be 48 kHz (it is " + std::to_string(rate) + " Hz)";
    return false;
  }
  struct Pair {
    const char *name;
    int left, right;
  };
  static const Pair hesuvi[] = {{"FL", 0, 1}, {"SL", 2, 3}, {"BL", 4, 5}, {"FC", 6, 13}, {"FR", 8, 7}, {"SR", 10, 9}, {"BR", 12, 11}};
  static const Pair bloob[] = {{"FL", 0, 1}, {"FR", 2, 3}, {"FC", 4, 5}, {"BL", 8, 9}, {"BR", 10, 11}, {"SL", 12, 13}, {"SR", 14, 15}};
  const Pair *table = ch == 14 ? hesuvi : ch == 16 ? bloob : nullptr;
  const int count = 7;
  if (!table) {
    *error = "the response needs 14 (HeSuVi) or 16 channels, not " + std::to_string(ch);
    return false;
  }
  *frames = static_cast<int>(s.size() / static_cast<size_t>(ch));
  size_t n = speakers.size() * 2;
  out->assign(static_cast<size_t>(*frames) * n, 0.0f);
  for (size_t k = 0; k < speakers.size(); k++) {
    std::string name = speakers[k].name;
    // A 5.1 render's side speakers use the file's surround pair; stereo uses the fronts.
    const Pair *pair = nullptr;
    for (int i = 0; i < count; i++)
      if (name == table[i].name) pair = &table[i];
    if (!pair) continue;  // LFE: mixed by the filter
    float gain = std::pow(10.0f, speakers[k].gain_db / 20);
    for (int t = 0; t < *frames; t++) {
      (*out)[static_cast<size_t>(t) * n + 2 * k] = gain * s[static_cast<size_t>(t * ch + pair->left)];
      (*out)[static_cast<size_t>(t) * n + 2 * k + 1] = gain * s[static_cast<size_t>(t * ch + pair->right)];
    }
  }
  return true;
}

}  // namespace

std::string HeadphoneOptions::summary() const {
  std::string s = !label.empty() ? label : ir_file.empty() ? binaural::room_named(room).name : "Measured response";
  if (xm5) s += " \xC2\xB7 XM5";
  if (night) s += " \xC2\xB7 Night";
  if (boost_db != 0) s += " \xC2\xB7 +" + num(boost_db) + " dB";
  return s;
}

std::string HeadphoneChain::tail_filters(const HeadphoneOptions &o) {
  std::string f;
  if (o.xm5) {
    f += "volume=" + num(kXm5Preamp) + "dB";
    for (const Band &b : kXm5)
      f += std::string(",") + b.type + "=f=" + num(b.f) + ":t=q:w=" + num(b.q) + ":g=" + num(b.gain);
  }
  // Night: lift the quiet, hold back the loud (about 4:1 above -20 dBFS).
  if (o.night) f += std::string(f.empty() ? "" : ",") + "acompressor=threshold=0.1:ratio=4:attack=10:release=250:makeup=2";
  f += std::string(f.empty() ? "" : ",") + "volume=" + num(o.boost_db) + "dB";
  // A look-ahead limiter at -1 dBFS: loud, never clipped. Its delay is compensated.
  f += ",alimiter=limit=0.89:attack=5:release=80:level=0:latency=1";
  return f;
}

void HeadphoneChain::close() {
  avfilter_graph_free(&graph_);
  in_ = out_ = nullptr;
  layout_.clear();
}

bool HeadphoneChain::open(const HeadphoneOptions &o, const AVChannelLayout *layout, int rate, int sample_format, std::string *error) {
  close();
  // The headphone filter warns of every sample above full scale; the samples are
  // float and the limiter after it brings them back, so the warnings are noise.
  av_log_set_level(AV_LOG_ERROR);
  int in_ch = layout->nb_channels;
  layout_ = in_ch >= 7 ? "7.1" : in_ch >= 3 ? "5.1(side)" : "stereo";
  std::vector<binaural::Speaker> speakers = binaural::layout_speakers(layout_);
  for (auto &sp : speakers)
    if (sp.name == "FC") sp.gain_db = o.dialogue_db;

  // The responses, interleaved: computed, or the owner's measured file.
  int frames = 0;
  std::vector<float> ir;
  size_t n = speakers.size() * 2;
  bool have = false;
  if (!o.ir_file.empty()) {
    std::string why;
    have = measured(o.ir_file, speakers, &frames, &ir, &why);
    if (!have) logf("headphones", "measured response not used: %s", why.c_str());
  }
  if (have) {
    // Level-match a measured response to Bloob's rooms: its loudest ear channel
    // gets the energy of Lounge's, so switching rooms doesn't jump in volume and
    // LFE (mixed by the filter at a fixed level) keeps its balance.
    binaural::Responses ref = binaural::compute(binaural::room_named("lounge"), speakers, 48000);
    double want = 0, got = 0;
    for (const auto &ch : ref.channels) {
      double e = 0;
      for (float v : ch) e += static_cast<double>(v) * v;
      want = std::max(want, e);
    }
    for (size_t c = 0; c < n; c++) {
      double e = 0;
      for (int t = 0; t < frames; t++) e += static_cast<double>(ir[static_cast<size_t>(t) * n + c]) * ir[static_cast<size_t>(t) * n + c];
      got = std::max(got, e);
    }
    if (got > 0 && want > 0) {
      float scale = static_cast<float>(std::sqrt(want / got));
      for (auto &v : ir) v *= scale;
    }
  }
  if (!have) {
    binaural::Responses r = binaural::compute(binaural::room_named(o.room), speakers, 48000);
    frames = r.length;
    ir.assign(static_cast<size_t>(frames) * n, 0.0f);
    for (size_t c = 0; c < n; c++)
      for (int t = 0; t < frames; t++) ir[static_cast<size_t>(t) * n + c] = r.channels[c][static_cast<size_t>(t)];
  }

  char in_desc[128] = {0}, ir_desc[128] = {0};
  av_channel_layout_describe(layout, in_desc, sizeof in_desc);
  AVChannelLayout ir_layout;
  av_channel_layout_default(&ir_layout, static_cast<int>(n));
  av_channel_layout_describe(&ir_layout, ir_desc, sizeof ir_desc);
  std::string map;
  for (const auto &sp : speakers) map += (map.empty() ? "" : "|") + sp.name;
  // The headphone filter lowers everything by 3 dB per input channel; gain undoes it.
  // LFE plays 10 dB up (as on speakers), matched to the responses' own level.
  int render_ch = static_cast<int>(speakers.size());
  double lfe_db = 10 + 20 * std::log10(0.45);
  std::string desc =
      "abuffer@in=time_base=1/" + std::to_string(rate) + ":sample_rate=" + std::to_string(rate) +
      ":sample_fmt=" + av_get_sample_fmt_name(static_cast<AVSampleFormat>(sample_format)) + ":channel_layout=" + in_desc + "[a];" +
      "abuffer@ir=time_base=1/48000:sample_rate=48000:sample_fmt=flt:channel_layout=" + ir_desc + "[ir];" +
      "[a]aresample=48000,aformat=sample_rates=48000:channel_layouts=" + layout_ + "[m];" +
      "[m][ir]headphone=map=" + map + ":hrir=multich:type=freq:size=4096:gain=" + num(3.0 * render_ch) +
      ":lfe=" + num(lfe_db) + "," + tail_filters(o) +
      ",aformat=sample_fmts=fltp:sample_rates=48000:channel_layouts=stereo,abuffersink@out";

  graph_ = avfilter_graph_alloc();
  AVFilterInOut *ins = nullptr, *outs = nullptr;
  int r = avfilter_graph_parse2(graph_, desc.c_str(), &ins, &outs);
  avfilter_inout_free(&ins);
  avfilter_inout_free(&outs);
  if (r >= 0) r = avfilter_graph_config(graph_, nullptr);
  in_ = r >= 0 ? avfilter_graph_get_filter(graph_, "abuffer@in") : nullptr;
  AVFilterContext *irsrc = r >= 0 ? avfilter_graph_get_filter(graph_, "abuffer@ir") : nullptr;
  out_ = r >= 0 ? avfilter_graph_get_filter(graph_, "abuffersink@out") : nullptr;
  if (!in_ || !irsrc || !out_) {
    char why[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(r, why, sizeof why);
    if (error) *error = std::string("headphone graph: ") + why;
    logf("headphones", "could not build the chain: %s (%s)", why, desc.c_str());
    close();
    return false;
  }
  // The responses go in once, then their input ends.
  AVFrame *f = av_frame_alloc();
  f->format = AV_SAMPLE_FMT_FLT;
  f->sample_rate = 48000;
  f->nb_samples = frames;
  av_channel_layout_copy(&f->ch_layout, &ir_layout);
  f->pts = 0;
  bool ok = av_frame_get_buffer(f, 0) >= 0;
  if (ok) {
    std::memcpy(f->data[0], ir.data(), ir.size() * sizeof(float));
    ok = av_buffersrc_add_frame(irsrc, f) >= 0 && av_buffersrc_add_frame(irsrc, nullptr) >= 0;
  }
  av_frame_free(&f);
  av_channel_layout_uninit(&ir_layout);
  if (!ok) {
    if (error) *error = "headphone responses refused";
    close();
    return false;
  }
  logf("headphones", "%s render of %s: %s, %d-sample responses%s", layout_.c_str(), in_desc, o.summary().c_str(), frames,
       have ? " (measured)" : "");
  return true;
}

bool HeadphoneChain::push(AVFrame *frame) {
  if (!in_) return false;
  return av_buffersrc_add_frame_flags(in_, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0;
}

bool HeadphoneChain::pull(AVFrame *out) {
  if (!out_) return false;
  return av_buffersink_get_frame(out_, out) >= 0;
}
