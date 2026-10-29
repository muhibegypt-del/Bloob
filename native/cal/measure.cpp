#include "measure.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace cal {

static bool agree(const Vec3 &a, const Vec3 &b) {
  double y = std::max(a.y, b.y);
  if (y > 5) {
    double xa, ya, xb, yb;
    chromaticity(a, &xa, &ya);
    chromaticity(b, &xb, &yb);
    return std::fabs(a.y - b.y) / y <= 0.005 && std::fabs(xa - xb) <= 0.0015 && std::fabs(ya - yb) <= 0.0015;
  }
  return std::fabs(a.y - b.y) <= 0.02;
}

static Vec3 median(const std::vector<Vec3> &v) {
  Vec3 m;
  for (int c = 0; c < 3; c++) {
    std::vector<double> x;
    for (const Vec3 &r : v) x.push_back(r[c]);
    std::sort(x.begin(), x.end());
    m[c] = x.size() % 2 ? x[x.size() / 2] : 0.5 * (x[x.size() / 2 - 1] + x[x.size() / 2]);
  }
  return m;
}

bool Measurer::agreed(const Patch &p, Vec3 *XYZ, int min_reads) {
  std::vector<Vec3> got;
  int limit = std::max(5, min_reads + 2);
  for (int i = 0; i < limit; i++) {
    Vec3 r;
    if (!dev_.measure(p, &r)) return false;
    raw_count_++;
    got.push_back(r);
    size_t need = static_cast<size_t>(std::max(r.y < 0.5 ? 4 : 2, min_reads));
    if (got.size() >= need && agree(got[got.size() - 2], got.back())) break;
    if (i == limit - 1) noisy_++;
  }
  *XYZ = median(got);
  return true;
}

// The picture changed on purpose (a new table or light), so the sentinel gets a
// new reference. Drift seen so far in the old state is carried over: dimming
// doesn't stop because a table changed.
bool Measurer::rebase() {
  if (ref_y_ > 0 && last_y_ > 0) carried_ *= ref_y_ / last_y_;
  Vec3 w;
  if (!agreed(sentinel_, &w)) return false;
  ref_y_ = last_y_ = w.y;
  factor_ = carried_;
  since_ = 0;
  return true;
}

bool Measurer::read(const Patch &p, Vec3 *XYZ, int min_reads) {
  if (ref_y_ > 0 && ++since_ >= 20) {
    Vec3 w;
    if (!agreed(sentinel_, &w)) return false;
    if (last_y_ > 0 && std::fabs(w.y / last_y_ - 1) > 0.015) jumps_++;
    last_y_ = w.y;
    factor_ = carried_ * ref_y_ / std::max(w.y, 1e-9);
    since_ = 0;
  }
  if (!agreed(p, XYZ, min_reads)) return false;
  *XYZ = *XYZ * factor_;
  return true;
}

}  // namespace cal
