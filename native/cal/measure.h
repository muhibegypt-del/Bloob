#pragma once
// Reading rules (CALIBRATION-PLAN.md §4.3): every reading the procedure uses
// passes through here. Repeats until two consecutive readings agree (more near
// black), keeps the median, and normalises slow dimming with a sentinel patch.
#include <functional>
#include <string>

#include "device.h"

namespace cal {

// Every line the procedure logs is also passed here (bloob-cal shows them live).
extern std::function<void(const std::string &)> g_log_hook;

class Measurer {
 public:
  explicit Measurer(Device &d) : dev_(d) {}
  // `min_reads`: at least this many readings (the median is kept), for precise stages;
  // 0 is the default (2, or 1 with set_single).
  bool read(const Patch &p, Vec3 *XYZ, int min_reads = 0);
  // The sentinel (white in the mode's range); `rebase` takes its reference now.
  void set_sentinel(const Patch &p) { sentinel_ = p; }
  bool rebase();
  // One reading per patch (two near black) unless a stage asks for more, as Calman and
  // ColourSpace take them: the fitted model averages the noise instead (quick runs).
  void set_single(bool on) { single_ = on; }
  long readings() const { return raw_count_; }
  int noisy() const { return noisy_; }
  int jumps() const { return jumps_; }

 private:
  bool agreed(const Patch &p, Vec3 *XYZ, int min_reads = 0);
  Device &dev_;
  Patch sentinel_{940, 940, 940};
  double ref_y_ = 0, last_y_ = 0, factor_ = 1;
  double carried_ = 1;  // drift from earlier states, kept across rebases
  int since_ = 0, noisy_ = 0, jumps_ = 0;
  bool single_ = false;
  long raw_count_ = 0;
};

}  // namespace cal
