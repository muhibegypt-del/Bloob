#pragma once
// A greyscale report in the manner of HCFR (4.0.1): measured 0-100% grey
// against SDR BT.709 with a plain power-law gamma (2.4 by default) at the
// measured white and black, D65 white point. Per level: luminance and its
// target, the gamma at that point, RGB balance (100/100/100 is D65 at that
// level), correlated colour temperature, and CIEDE2000 with and without the
// luminance error. Pure: the measuring is the caller's (bloob-cal greyscale).
#include <string>
#include <vector>

#include "colour.h"

namespace cal {

struct GreyRow {
  double pct = 0;     // stimulus, 0..100
  Vec3 xyz;           // measured, absolute (cd/m2)
  // Filled by greyscale_report():
  double target_y = 0, gamma = 0, cct = 0, x = 0, y = 0;
  double r = 0, g = 0, b = 0;  // RGB balance, %
  double de = 0, de_colour = 0;
};

struct GreySummary2 {
  double white_y = 0, black_y = 0, contrast = 0;
  double avg_gamma = 0, avg_de = 0, max_de = 0, max_de_pct = 0, avg_de_colour = 0;
  double white_cct = 0;
};

// Fills each row's derived values; `gamma` is the target power. Rows must
// include 100% (the white the targets are scaled to).
GreySummary2 greyscale_report(std::vector<GreyRow> &rows, double gamma);
// Correlated colour temperature from chromaticity (McCamy).
double cct_mccamy(double x, double y);
std::string greyscale_csv(const std::vector<GreyRow> &rows);
std::string greyscale_json(const std::vector<GreyRow> &rows, const GreySummary2 &s, double gamma, const std::string &when);
// A self-contained HTML page: summary, the table, and charts of luminance,
// gamma, RGB balance and ΔE (inline SVG, no scripts).
std::string greyscale_html(const std::vector<GreyRow> &rows, const GreySummary2 &s, double gamma, const std::string &when);

}  // namespace cal
