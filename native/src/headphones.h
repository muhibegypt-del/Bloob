#pragma once
// Headphones mode (3.2): the film's own channels (5.1 or 7.1) rendered for two
// ears, so a Bluetooth headset hears speakers around it rather than a
// downmix inside the head. One FFmpeg filter graph, run on the engine's
// feeder thread after the sound is decoded:
//   1. the channels as they are (dialogue normalisation skipped and dynamic
//      range compression off, set on the decoder: loud and full);
//   2. each speaker through Bloob's binaural room (binaural.h), or through a
//      measured response the owner put in the data folder; dialogue raised a little;
//   3. the correction for Sony WF-1000XM5 earbuds (AutoEQ, oratory1990's
//      measurement), since a binaural render assumes neutral headphones;
//   4. Night: gentle compression, for quiet listening;
//   5. the volume boost, then a limiter that never lets a peak clip;
//   6. stereo, 48 kHz, for the E-AC-3 encoder.
#include <string>

extern "C" {
struct AVFrame;
struct AVChannelLayout;
struct AVFilterGraph;
struct AVFilterContext;
}

struct HeadphoneOptions {
  bool enabled = false;
  std::string room = "lounge";  // binaural::rooms() id
  float dialogue_db = 3;        // centre channel lift
  float boost_db = 9;           // after the render, before the limiter
  bool night = false;           // compress loud passages; the decoder's own DRC on too
  bool xm5 = true;              // the WF-1000XM5 correction
  int lead_ms = 0;              // sound sent this much ahead of the picture (Bluetooth delay)
  // A measured response (WAV, 48 kHz): 14 channels in HeSuVi's order, or 16 in
  // Bloob's (FL FR FC LFE BL BR SL SR, each left ear then right). Empty: the room above.
  std::string ir_file;
  std::string label;  // for people: the room's or the measured response's name

  bool operator==(const HeadphoneOptions &o) const {
    return enabled == o.enabled && room == o.room && dialogue_db == o.dialogue_db && boost_db == o.boost_db &&
           night == o.night && xm5 == o.xm5 && lead_ms == o.lead_ms && ir_file == o.ir_file;
  }
  bool operator!=(const HeadphoneOptions &o) const { return !(*this == o); }
  // For people: "Lounge · XM5 · +9 dB".
  std::string summary() const;
};

class HeadphoneChain {
 public:
  HeadphoneChain() = default;
  ~HeadphoneChain() { close(); }
  HeadphoneChain(const HeadphoneChain &) = delete;
  HeadphoneChain &operator=(const HeadphoneChain &) = delete;

  // Builds the graph for decoded frames of this layout, rate and sample format.
  bool open(const HeadphoneOptions &options, const AVChannelLayout *layout, int rate, int sample_format, std::string *error);
  void close();
  bool is_open() const { return graph_ != nullptr; }
  // A decoded frame in (null: end of stream). Its pts is in 1/rate units.
  bool push(AVFrame *frame);
  // A rendered frame out: fltp, 48 kHz, stereo. False when none is ready.
  bool pull(AVFrame *out);
  // Which layout the render uses ("7.1", "5.1(side)", "stereo"), for people.
  const std::string &layout() const { return layout_; }

  // The filter description after the binaural stage (exposed for tests).
  static std::string tail_filters(const HeadphoneOptions &options);

 private:
  AVFilterGraph *graph_ = nullptr;
  AVFilterContext *in_ = nullptr, *out_ = nullptr;
  std::string layout_;
};
