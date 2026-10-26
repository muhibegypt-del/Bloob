// Headphones mode previews (3.2), for listening on a phone with the same
// earbuds before a TV install: a 7.1 speaker walk and a film excerpt, each
// through every room, plus the plain stereo downmix for comparison.
// Also reports how much faster than real time the render runs.
//   hp_preview <out-dir> [film-file]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

#include "binaural.h"
#include "headphones.h"

namespace {

// 16-bit stereo WAV.
struct Wav {
  FILE *f = nullptr;
  uint32_t frames = 0;
  explicit Wav(const std::string &path) {
    f = std::fopen(path.c_str(), "wb");
    uint8_t header[44] = {0};
    if (f) std::fwrite(header, 1, 44, f);
  }
  void write(const float *l, const float *r, int n) {
    for (int i = 0; i < n; i++) {
      int16_t s[2] = {static_cast<int16_t>(std::lround(std::max(-1.0f, std::min(1.0f, l[i])) * 32767)),
                      static_cast<int16_t>(std::lround(std::max(-1.0f, std::min(1.0f, r[i])) * 32767))};
      std::fwrite(s, 2, 2, f);
    }
    frames += static_cast<uint32_t>(n);
  }
  ~Wav() {
    if (!f) return;
    auto u32 = [](uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = static_cast<uint8_t>(v >> (8 * i)); };
    uint8_t h[44];
    std::memcpy(h, "RIFF\0\0\0\0WAVEfmt \x10\0\0\0\x01\0\x02\0", 24);
    u32(h + 4, 36 + frames * 4);
    u32(h + 24, 48000);
    u32(h + 28, 48000 * 4);
    h[32] = 4;
    h[33] = 0;
    h[34] = 16;
    h[35] = 0;
    std::memcpy(h + 36, "data", 4);
    u32(h + 40, frames * 4);
    std::fseek(f, 0, SEEK_SET);
    std::fwrite(h, 1, 44, f);
    std::fclose(f);
  }
};

// Renders planar float frames (any layout) through a chain into a WAV.
struct Render {
  HeadphoneChain chain;
  Wav wav;
  AVFrame *out = av_frame_alloc();
  double seconds = 0;
  Render(const std::string &path) : wav(path) {}
  ~Render() { av_frame_free(&out); }
  bool open(const HeadphoneOptions &o, const AVChannelLayout *layout, int rate, int fmt) {
    std::string error;
    if (!chain.open(o, layout, rate, fmt, &error)) {
      std::printf("chain: %s\n", error.c_str());
      return false;
    }
    return true;
  }
  void push(AVFrame *f) {
    auto t0 = std::chrono::steady_clock::now();
    chain.push(f);
    while (chain.pull(out)) {
      wav.write(reinterpret_cast<float *>(out->extended_data[0]), reinterpret_cast<float *>(out->extended_data[1]), out->nb_samples);
      av_frame_unref(out);
    }
    seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  }
};

// The ITU stereo downmix, as a TV would play the same channels.
void downmix(AVFrame *f, Wav &wav) {
  int n = f->nb_samples, ch = f->ch_layout.nb_channels;
  std::vector<float> l(static_cast<size_t>(n)), r(static_cast<size_t>(n));
  for (int c = 0; c < ch; c++) {
    enum AVChannel id = av_channel_layout_channel_from_index(&f->ch_layout, static_cast<unsigned>(c));
    float gl = 0, gr = 0;
    switch (id) {
      case AV_CHAN_FRONT_LEFT: gl = 1; break;
      case AV_CHAN_FRONT_RIGHT: gr = 1; break;
      case AV_CHAN_FRONT_CENTER: gl = gr = 0.707f; break;
      case AV_CHAN_SIDE_LEFT: case AV_CHAN_BACK_LEFT: gl = 0.707f; break;
      case AV_CHAN_SIDE_RIGHT: case AV_CHAN_BACK_RIGHT: gr = 0.707f; break;
      default: break;
    }
    const float *d = reinterpret_cast<const float *>(f->extended_data[c]);
    for (int i = 0; i < n; i++) {
      l[static_cast<size_t>(i)] += gl * d[i] * 0.5f;
      r[static_cast<size_t>(i)] += gr * d[i] * 0.5f;
    }
  }
  wav.write(l.data(), r.data(), n);
}

// Pink noise bursts going round the 7.1 speakers: front left, centre, front
// right, side right, back right, back left, side left.
void speaker_walk(const std::string &dir) {
  AVChannelLayout layout = AV_CHANNEL_LAYOUT_7POINT1;
  const AVChannel order[] = {AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_CENTER, AV_CHAN_FRONT_RIGHT, AV_CHAN_SIDE_RIGHT,
                             AV_CHAN_BACK_RIGHT, AV_CHAN_BACK_LEFT, AV_CHAN_SIDE_LEFT};
  const int block = 1024, per_speaker = 48000 * 3 / 2;
  std::vector<Render *> renders;
  std::vector<std::string> names;
  for (const auto &room : binaural::rooms()) {
    HeadphoneOptions o;
    o.enabled = true;
    o.room = room.id;
    auto *r = new Render(dir + "/walk-" + room.id + ".wav");
    if (!r->open(o, &layout, 48000, AV_SAMPLE_FMT_FLTP)) return;
    renders.push_back(r);
  }
  Wav plain(dir + "/walk-tv-downmix.wav");
  AVFrame *f = av_frame_alloc();
  uint32_t state = 7;
  float b[7] = {0};
  long long pts = 0;
  for (AVChannel speaker : order) {
    int idx = av_channel_layout_index_from_channel(&layout, speaker);
    for (int done = 0; done < per_speaker; done += block) {
      f->format = AV_SAMPLE_FMT_FLTP;
      f->sample_rate = 48000;
      f->nb_samples = block;
      av_channel_layout_copy(&f->ch_layout, &layout);
      f->pts = pts;
      av_frame_get_buffer(f, 0);
      for (int c = 0; c < 8; c++) std::memset(f->extended_data[c], 0, block * sizeof(float));
      float *d = reinterpret_cast<float *>(f->extended_data[idx]);
      for (int i = 0; i < block; i++) {
        int t = done + i;
        // Three 300 ms bursts with soft edges, then silence.
        int in_burst = t % 14400;
        float env = t < 43200 && in_burst < 9600 ? std::sin(3.14159f * in_burst / 9600) : 0;
        state = state * 1664525u + 1013904223u;
        float white = static_cast<float>(state >> 8) / 8388608.0f - 1.0f;
        // Paul Kellet's pink filter.
        b[0] = 0.99886f * b[0] + white * 0.0555179f;
        b[1] = 0.99332f * b[1] + white * 0.0750759f;
        b[2] = 0.96900f * b[2] + white * 0.1538520f;
        b[3] = 0.86650f * b[3] + white * 0.3104856f;
        b[4] = 0.55000f * b[4] - white * 0.5329522f;
        b[5] = -0.7616f * b[5] - white * 0.0168980f;
        float pink = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + white * 0.5362f;
        b[6] = white * 0.115926f;
        d[i] = 0.08f * env * pink;  // about -20 dBFS, like a speaker calibration tone
      }
      for (auto *r : renders) r->push(f);
      downmix(f, plain);
      av_frame_unref(f);
      pts += block;
    }
  }
  av_frame_free(&f);
  for (size_t i = 0; i < renders.size(); i++) {
    std::printf("walk %-7s rendered %.1fx faster than real time\n", binaural::rooms()[i].id,
                (7 * 1.5) / std::max(1e-6, renders[i]->seconds));
    delete renders[i];
  }
}

// A film's first sound track, decoded and rendered through every room.
void film(const std::string &dir, const std::string &path) {
  AVFormatContext *fmt = nullptr;
  if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0 || avformat_find_stream_info(fmt, nullptr) < 0) {
    std::printf("cannot read %s\n", path.c_str());
    return;
  }
  int s = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (s < 0) return;
  const AVCodec *dc = avcodec_find_decoder(fmt->streams[s]->codecpar->codec_id);
  AVCodecContext *dec = avcodec_alloc_context3(dc);
  avcodec_parameters_to_context(dec, fmt->streams[s]->codecpar);
  AVDictionary *opts = nullptr;
  av_dict_set(&opts, "drc_scale", "0", 0);
  avcodec_open2(dec, dc, &opts);
  av_dict_free(&opts);
  std::vector<Render *> renders;
  Wav plain(dir + "/film-tv-downmix.wav");
  AVPacket *pkt = av_packet_alloc();
  AVFrame *f = av_frame_alloc();
  double audio_s = 0;
  while (av_read_frame(fmt, pkt) >= 0) {
    if (pkt->stream_index == s && avcodec_send_packet(dec, pkt) >= 0) {
      while (avcodec_receive_frame(dec, f) == 0) {
        if (renders.empty()) {
          for (const auto &room : binaural::rooms()) {
            HeadphoneOptions o;
            o.enabled = true;
            o.room = room.id;
            auto *r = new Render(dir + "/film-" + room.id + ".wav");
            if (!r->open(o, &f->ch_layout, f->sample_rate, f->format)) return;
            renders.push_back(r);
          }
        }
        audio_s += static_cast<double>(f->nb_samples) / f->sample_rate;
        for (auto *r : renders) r->push(f);
        if (f->format == AV_SAMPLE_FMT_FLTP && f->sample_rate == 48000) downmix(f, plain);
        av_frame_unref(f);
      }
    }
    av_packet_unref(pkt);
  }
  for (size_t i = 0; i < renders.size(); i++) {
    std::printf("film %-7s %s, %.0f s rendered %.1fx faster than real time\n", binaural::rooms()[i].id,
                renders[i]->chain.layout().c_str(), audio_s, audio_s / std::max(1e-6, renders[i]->seconds));
    delete renders[i];
  }
  av_frame_free(&f);
  av_packet_free(&pkt);
  avcodec_free_context(&dec);
  avformat_close_input(&fmt);
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::printf("usage: hp_preview <out-dir> [film-file]\n");
    return 1;
  }
  speaker_walk(argv[1]);
  if (argc > 2) film(argv[1], argv[2]);
  return 0;
}
