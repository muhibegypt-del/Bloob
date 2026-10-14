#pragma once
// Interaction timing for the log and Diagnostics: key → present, and render cost.
// "Present" is when SDL_RenderPresent returns, not when light leaves the panel.
#include <algorithm>
#include <string>
#include <vector>

class PerfWindow {
 public:
  void key(long long at_ms) {
    if (!pending_key_) pending_key_ = at_ms;
  }
  // After SDL_RenderPresent. `render_ms` covers draw and present.
  void frame(long long now_ms, long long render_ms) {
    if (!started_) started_ = now_ms;
    frames_.push_back(render_ms);
    if (pending_key_) {
      keys_.push_back(std::max(0LL, now_ms - pending_key_));
      pending_key_ = 0;
    }
  }
  bool due(long long now_ms) const { return started_ && now_ms - started_ >= 5000; }
  // Summarises the window and starts a new one.
  std::string flush(long long now_ms) {
    long long slow = std::count_if(frames_.begin(), frames_.end(), [](long long v) { return v > 33; });
    std::string line = "keys n=" + std::to_string(keys_.size()) + " key->present " + stats(keys_) + "; frames n=" +
                       std::to_string(frames_.size()) + " render " + stats(frames_) + " slow>33ms=" + std::to_string(slow);
    last_render_p95_ = pct(frames_, 95);
    last_frames_ = frames_.size();
    last_line_ = line;
    keys_.clear();
    frames_.clear();
    started_ = now_ms;
    return line;
  }
  long long last_render_p95() const { return last_render_p95_; }
  size_t last_frames() const { return last_frames_; }
  const std::string &last_line() const { return last_line_; }

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
  std::vector<long long> keys_, frames_;
  std::string last_line_;
};
