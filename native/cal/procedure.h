#pragma once
// The calibration procedure (CALIBRATION-PLAN.md §4–7), written once and run
// against any Device: the virtual G2 in tests, the real TV through the service.
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "device.h"
#include "grey.h"
#include "lut.h"
#include "measure.h"
#include "model.h"

namespace cal {

struct Stats {
  double grey_avg = 0, grey_max = 0, colour_avg = 0, colour_max = 0;
  int grey_n = 0, colour_n = 0;
  std::string str() const;
  bool worse_than(const Stats &o) const;
};

struct SdrTarget {
  double Lw = 100;   // white, cd/m²
  double Lb = 0;     // black for BT.1886 (the procedure fills in the measured black)
  bool bt1886 = true;
  double gamma = 2.4;  // when bt1886 is false
  Primaries primaries = kRec709;
  Vec3 white() const { return xyY(primaries.wx, primaries.wy, Lw); }
  double eotf(double v) const;
  Vec3 xyz(Vec3 signal) const;
};

// Limited-range 10-bit codes.
inline int code_limited(double v) { return 64 + static_cast<int>(std::lround(876 * std::clamp(v, 0.0, 1.0))); }
inline double signal_limited(int code) { return (code - 64) / 876.0; }

struct SdrOptions {
  SdrTarget target;
  int order = -1;            // -1: detect (C2.3); 0: 3D then 1D; 1: 1D then 3D
  int grey_passes = 12;
  int colour_passes = 3;
  int lattice = 11;
  int grid = 9;              // characterisation grid per axis
  unsigned seed = 7;
};

struct SdrResult {
  bool ok = false;
  std::string error;
  bool lut1d_first = false;
  Stats baseline, predicted, final;
  int light = 0;
  Lut1D lut1d = Lut1D::unity();
  Lut3D lut3d = Lut3D::unity();
  long readings = 0;
  double model_holdout = 0;
  std::vector<std::string> log;
};

SdrResult run_sdr(Device &dev, const SdrOptions &options);

}  // namespace cal
