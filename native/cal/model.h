#pragma once
// The panel model (CALIBRATION-PLAN.md §6.2): from device RGB (what goes into
// the TV's tables) to measured XYZ, fitted to readings.
//   baseline: a tone curve per channel and the primaries' matrix;
//   residual: a smooth correction over the RGB cube (lattice regression with
//   a Laplacian penalty, its weight chosen by cross-validation), which carries
//   what a matrix cannot: the white sub-pixel, crosstalk, non-additivity.
#include <array>
#include <string>
#include <vector>

#include "colour.h"

namespace cal {

struct Sample {
  Vec3 rgb;  // device RGB in [0, 1]
  Vec3 XYZ;  // measured, cd/m²
  double weight = 1;
};

// Monotone cubic (Fritsch–Carlson) through points sorted by x.
class Monotone {
 public:
  void fit(std::vector<double> x, std::vector<double> y);
  double operator()(double x) const;
  double slope(double x) const;
  bool empty() const { return x_.empty(); }

 private:
  std::vector<double> x_, y_, m_;
};

class PanelModel {
 public:
  // Needs the three channel ramps (samples with the other two channels at 0).
  bool fit(const std::vector<Sample> &samples, int lattice, std::string *error);
  Vec3 predict(Vec3 rgb) const;
  // Device RGB in [0,1]³ whose prediction is closest to `target` (ΔE2000 against `white`).
  // Returns the ΔE reached; `rgb` holds the start and receives the answer.
  double invert(const Vec3 &target, const Vec3 &white, Vec3 *rgb) const;
  // Leave-out error of the last fit (average ΔE2000 on held-out samples), for reports.
  double holdout_de() const { return holdout_de_; }
  double lambda() const { return lambda_; }

 private:
  Vec3 baseline(Vec3 rgb) const;
  Vec3 residual(Vec3 rgb) const;
  bool fit_lattice(const std::vector<Sample> &s, const std::vector<int> &use, double lambda, std::vector<Vec3> *out) const;

  Vec3 black_;
  Mat3 M_;
  std::array<Monotone, 3> trc_;
  int n_ = 11;
  std::vector<Vec3> lat_;
  double scale_ = 1;  // white Y: the residual is fitted relative to it
  double lambda_ = 0, holdout_de_ = 0;
};

}  // namespace cal
