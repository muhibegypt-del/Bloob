#pragma once
// HDR10 and Dolby Vision calibration (CALIBRATION-PLAN.md §7.4 and §7.5),
// against any Device: the TV must already be showing the mode's signal (Bloob's
// carrier clip) and be in the target picture mode.
#include <string>
#include <vector>

#include "procedure.h"

namespace cal {

struct HdrOptions {
  double grey_top = 0.9;  // the grey scale is corrected up to this share of the peak
  bool matrix_lut = true; // Calman's five-patch matrix LUT for colour (kept only if it helps)
  int grey_passes = 12;
  unsigned seed = 7;
};

struct HdrResult {
  bool ok = false;
  std::string error;
  double peak = 0;             // cd/m², 10% window
  bool matrix_kept = false;
  Stats baseline, without_matrix, with_matrix, final;  // ΔE ITP
  int tonemap[7] = {0, 0, 0, 0, 0, 0, 0};
  bool highlights_monotonic = false;
  Lut1D lut1d = Lut1D::unity();
  Lut3D lut3d = Lut3D::unity();
  long readings = 0;
  std::vector<std::string> log;
};

HdrResult run_hdr10(Device &dev, const HdrOptions &options);

struct DolbyOptions {
  int grey_passes = 12;
  unsigned seed = 7;
};

struct DolbyResult {
  bool ok = false;
  std::string error;
  bool passthrough = false;     // patches reach the panel without Dolby's mapping (U7)
  bool config_loaded = false;   // the USB configuration was written and accepted
  double tmax = 0, tmin = 0;    // what the configuration says
  double peak_after = 0;        // the peak measured after the grey fix
  bool peak_matches = false;    // within 3% of Tmax
  bool black_ok = false;
  std::string config;           // DolbyVision_UserDisplayConfiguration.txt
  Stats baseline, final;        // ΔE2000 relative to Tmax
  Lut1D lut1d = Lut1D::unity();
  long readings = 0;
  std::vector<std::string> log;
};

DolbyResult run_dolby(Device &dev, const DolbyOptions &options);

// The 2019-format configuration block for one Dolby Vision picture mode (bscpylgtv's
// layout, lines ending in CRLF). Primaries: xr yr xg yg xb yb (white is D65).
std::string dolby_config(int picture_mode, double tmax, double tmin, double gamma, const double primaries[6]);

}  // namespace cal
