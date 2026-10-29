#pragma once
// Reading rules (CALIBRATION-PLAN.md §4.3): every reading the procedure uses
// passes through here. Repeats until two consecutive readings agree (more near
// black), keeps the median, and normalises slow dimming with a sentinel patch.
#include "device.h"

namespace cal {

class Measurer {
 public:
  explicit Measurer(Device &d) : dev_(d) {}
  // `min_reads`: at least this many readings (the median is kept), for precise stages.
  bool read(const Patch &p, Vec3 *XYZ, int min_reads = 2);
  // The sentinel (white in the mode's range); `rebase` takes its reference now.
  void set_sentinel(const Patch &p) { sentinel_ = p; }
  bool rebase();
  long readings() const { return raw_count_; }
  int noisy() const { return noisy_; }
  int jumps() const { return jumps_; }

 private:
  bool agreed(const Patch &p, Vec3 *XYZ, int min_reads = 2);
  Device &dev_;
  Patch sentinel_{940, 940, 940};
  double ref_y_ = 0, last_y_ = 0, factor_ = 1;
  double carried_ = 1;  // drift from earlier states, kept across rebases
  int since_ = 0, noisy_ = 0, jumps_ = 0;
  long raw_count_ = 0;
};

}  // namespace cal
