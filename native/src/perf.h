#pragma once
// Interaction and frame timing for the log, Diagnostics and the inspector.
// Milestones are named for what is measured (study O2):
//  - key->submit: a key press to the return of SDL_RenderPresent (the frame was
//    handed to the platform; not when light leaves the panel);
//  - per frame stages: update (state and animation), paint (drawing the
//    screens into the canvas), upload (artwork textures created this frame),
//    present (the final copy and SDL_RenderPresent).
#include <algorithm>
#include <string>
#include <vector>

class PerfWindow {
 public:
  struct Stages {
    long long update = 0, paint = 0, upload = 0, present = 0;
  };
  void key(long long at_ms) {
    if (!pending_key_) pending_key_ = at_ms;
  }
  // After SDL_RenderPresent. `render_ms` covers paint, upload and present.
  void frame(long long now_ms, long long render_ms, const Stages &s) {
    if (!started_) started_ = now_ms;
    frames_.push_back(render_ms);
    update_.push_back(s.update);
    paint_.push_back(s.paint);
    upload_.push_back(s.upload);
    present_.push_back(s.present);
    if (pending_key_) {
      keys_.push_back(std::max(0LL, now_ms - pending_key_));
      pending_key_ = 0;
    }
  }
  bool due(long long now_ms) const { return started_ && now_ms - started_ >= 5000; }
  // Summarises the window and starts a new one.
  std::string flush(long long now_ms) {
    long long slow = std::count_if(frames_.begin(), frames_.end(), [](long long v) { return v > 33; });
    std::string line = "keys n=" + std::to_string(keys_.size()) + " key->submit " + stats(keys_) + "; frames n=" +
                       std::to_string(frames_.size()) + " render " + stats(frames_) + " slow>33ms=" + std::to_string(slow) +
                       "; stages p95 update " + std::to_string(pct(update_, 95)) + " paint " + std::to_string(pct(paint_, 95)) +
                       " upload " + std::to_string(pct(upload_, 95)) + " present " + std::to_string(pct(present_, 95)) + " ms";
    last_render_p95_ = pct(frames_, 95);
    last_frames_ = frames_.size();
    last_line_ = line;
    last_.frames = frames_.size();
    last_.render_p50 = pct(frames_, 50);
    last_.render_p95 = pct(frames_, 95);
    last_.key_p50 = pct(keys_, 50);
    last_.key_p95 = pct(keys_, 95);
    last_.update_p95 = pct(update_, 95);
    last_.paint_p95 = pct(paint_, 95);
    last_.upload_p95 = pct(upload_, 95);
    last_.present_p95 = pct(present_, 95);
    last_.window_ms = now_ms - started_;
    keys_.clear();
    frames_.clear();
    update_.clear();
    paint_.clear();
    upload_.clear();
    present_.clear();
    started_ = now_ms;
    return line;
  }
  long long last_render_p95() const { return last_render_p95_; }
  size_t last_frames() const { return last_frames_; }
  const std::string &last_line() const { return last_line_; }

  struct Summary {
    size_t frames = 0;
    long long render_p50 = 0, render_p95 = 0, key_p50 = 0, key_p95 = 0;
    long long update_p95 = 0, paint_p95 = 0, upload_p95 = 0, present_p95 = 0, window_ms = 0;
  };
  const Summary &last() const { return last_; }

 private:
  static long long pct(std::vector<long long> v, int p) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, v.size() * static_cast<size_t>(p) / 100)];
  }
  static std::string stats(const std::vector<long long> &v) {
    if (v.empty()) return "-";
    return "p50 " + std::to_string(pct(v, 50)) + " p95 " + std::to_string(pct(v, 95)) + " max " +
           std::to_string(*std::max_element(v.begin(), v.end())) + " ms";
  }
  long long pending_key_ = 0, started_ = 0, last_render_p95_ = 0;
  size_t last_frames_ = 0;
  std::vector<long long> keys_, frames_, update_, paint_, upload_, present_;
  std::string last_line_;
  Summary last_;
};
