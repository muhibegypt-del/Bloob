#pragma once
// The grey-scale solve (CALIBRATION-PLAN.md §6.3), shared by SDR, HDR10 and
// Dolby Vision: measure greys, estimate each level's sensitivity, take damped
// Newton steps on the 1D LUT, keep the best table measured.
#include <functional>
#include <string>
#include <vector>

#include "measure.h"

namespace cal {

struct GreySpec {
  std::vector<int> codes;  // ascending grey levels (all three channels equal); the last is the top
  int code_lo = 64, code_hi = 940;  // where the ± sensitivity steps may go
  std::function<double(int)> signal;                         // code → 1D LUT input in [0, 1]
  std::function<Vec3(int)> target;                           // code → wanted XYZ, cd/m²
  std::function<double(const Vec3 &, const Vec3 &)> error;   // measured, wanted → ΔE
  int passes = 12;
  int step = 8;          // codes, for the sensitivity patches
  int jac_every = 1;     // measure sensitivities at every Nth level; the others borrow, scaled
  int jac_reads = 3;     // readings per sensitivity patch
  int grey_reads = 3;    // readings per grey level
  double done_below = 0; // stop as soon as a pass scores below this (0: run every pass)
};

struct GreyOutcome {
  bool ok = false;
  Lut1D lut;          // the best table measured (already on the TV)
  double reach = 1;   // share of the top target the panel can reach (< 1: more light needed)
  double score = 0;   // average ΔE of the levels, plus a quarter of the worst
};

// Starts from `start` (already uploaded). With `stop_if_unreachable`, returns after
// the first pass when the top target is out of reach, or at the end if the best table
// still left white more than 3% short, so the caller can add light.
GreyOutcome solve_grey(Device &dev, Measurer &meter, const GreySpec &spec, const Lut1D &start, bool stop_if_unreachable,
                       std::vector<std::string> *log);

}  // namespace cal
