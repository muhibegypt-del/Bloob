#pragma once
// Frame pacing and animation time (R5a). Pure: no SDL, unit-tested.
//
// Elements of A smooths frame time over the last three intervals with a floor
// derived from the display rate, and never runs more than one frame of work
// ahead of the display (resource study C1, C6). Bloob does the same:
//  - an animation step is the median of the last three present-to-present
//    intervals, never shorter than one refresh period and never longer than
//    100 ms, so one late frame does not make every animation jump;
//  - the first frame after an idle gap steps one refresh period, not the gap
//    (before 2.4.0 a focus move after a pause jumped 100 ms on its first frame);
//  - a frame is due at most once per refresh period, unless input arrived.
#include <algorithm>
#include <array>

class FramePacer {
 public:
  void set_refresh_hz(int hz) { refresh_ms_ = hz >= 24 && hz <= 240 ? 1000.0 / hz : 1000.0 / 60; }
  double refresh_ms() const { return refresh_ms_; }

  // Animation step, in seconds, for the loop iteration at `now`. `continuing`:
  // the previous iteration was part of a running animation.
  double step_seconds(long long now, bool continuing) {
    long long prev = last_step_;
    last_step_ = now;
    if (!continuing || prev < 0 || now - prev > kIdleGapMs) {
      count_ = 0;
      return refresh_ms_ / 1000.0;
    }
    double interval = static_cast<double>(now - prev);
    // Iterations closer together than a refresh (input between frames) step
    // their real interval and stay out of the frame-time history.
    if (interval < refresh_ms_ * 0.75) return interval / 1000.0;
    intervals_[next_] = interval;
    next_ = (next_ + 1) % intervals_.size();
    count_ = std::min<int>(count_ + 1, static_cast<int>(intervals_.size()));
    double median;
    if (count_ == 1) {
      median = intervals_[0];
    } else if (count_ == 2) {
      median = std::max(intervals_[0], intervals_[1]);  // the upper of two: never under-step
    } else {
      double a = intervals_[0], b = intervals_[1], c = intervals_[2];
      median = std::max(std::min(a, b), std::min(std::max(a, b), c));
    }
    return std::max(refresh_ms_, std::min(static_cast<double>(kIdleGapMs), median)) / 1000.0;
  }

  void presented(long long at) { last_present_ = at; }
  long long last_present() const { return last_present_; }

  // True when a new frame may start: one refresh period after the last (2 ms
  // early, for scheduling slack), or immediately when input is waiting.
  bool due(long long now, bool input) const {
    return input || last_present_ < 0 || now - last_present_ >= static_cast<long long>(refresh_ms_) - 2;
  }
  // Milliseconds until a frame is due (0 when due now).
  int wait_ms(long long now) const {
    if (last_present_ < 0) return 0;
    long long left = last_present_ + static_cast<long long>(refresh_ms_) - 2 - now;
    return left > 0 ? static_cast<int>(left) : 0;
  }

  static constexpr long long kIdleGapMs = 100;

 private:
  double refresh_ms_ = 1000.0 / 60;
  long long last_present_ = -1;
  long long last_step_ = -1;
  std::array<double, 3> intervals_{};
  size_t next_ = 0;
  int count_ = 0;
};
