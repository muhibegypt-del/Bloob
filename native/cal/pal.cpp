#include "pal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "model.h"

namespace cal {

std::vector<int> pal_levels() {
  std::vector<int> v;
  for (int p = 0; p <= 100; p += 5) v.push_back(p);
  return v;
}

double pal_table_x(int pct, TableIndex m) { return m == TableIndex::Signal ? pct / 100.0 : pal_code(pct) / 1023.0; }

Vec3 pal_target(const SdrTarget &t, int pct) {
  double v = pct / 100.0;
  return t.xyz({v, v, v});
}

GreySummary pal_summarise(std::vector<GreyLevel> &levels, const SdrTarget &t, const PalGoal &goal) {
  GreySummary s;
  Vec3 white = t.white();
  int n = 0;
  for (GreyLevel &l : levels) {
    l.target = pal_target(t, l.pct);
    l.scored = l.measured && l.pct >= goal.first_scored;
    l.de = l.measured ? de2000_xyz(l.XYZ, l.target, white) : 0;
    if (l.scored) {
      s.avg += l.de;
      n++;
      if (l.de > s.max) {
        s.max = l.de;
        s.max_at = l.pct;
      }
    }
    if (l.pct == 100 && l.measured) {
      s.white_Y = l.XYZ.y;
      s.white_err_pct = 100 * (l.XYZ.y / t.Lw - 1);
    }
  }
  if (n) s.avg /= n;
  s.goal_met = n > 0 && s.avg <= goal.avg && s.max <= goal.max && std::fabs(s.white_err_pct) <= goal.white_pct;
  return s;
}

bool pal_grey_sane(const Vec3 &XYZ, const Vec3 *below, double wobble_pct, std::string *why) {
  if (!std::isfinite(XYZ.x) || !std::isfinite(XYZ.y) || !std::isfinite(XYZ.z) || XYZ.y < 0) {
    if (why) *why = "not a number";
    return false;
  }
  if (below && XYZ.y < below->y * (1 - 3 * wobble_pct / 100) - 0.02) {
    if (why) *why = "darker than the level below";
    return false;
  }
  // Near neutral, once there is enough light for the chromaticity to mean something. A pink
  // or green flicker (LG's pattern transition, B1) or the wrong patch shows up here.
  if (XYZ.y >= 1.0 && duv(XYZ, xyY(kD65x, kD65y, 1)) > 0.03) {
    if (why) *why = "far from neutral";
    return false;
  }
  return true;
}

bool pal_adjust_valid(const PalAdjust &a, std::string *why) {
  auto fail = [&](const std::string &w) {
    if (why) *why = w;
    return false;
  };
  if (a.pct.size() < 2) return fail("fewer than two levels");
  for (size_t i = 0; i < a.pct.size(); i++) {
    if (a.pct[i] < 0 || a.pct[i] > 100) return fail("a level outside 0–100");
    if (i && a.pct[i] <= a.pct[i - 1]) return fail("levels not ascending");
  }
  for (int c = 0; c < 3; c++) {
    if (a.mult[c].size() != a.pct.size()) return fail(std::string("channel ") + "RGB"[c] + " has a different number of values than levels");
    for (double m : a.mult[c])
      if (!std::isfinite(m) || m < 0.5 || m > 1.5) return fail(std::string("channel ") + "RGB"[c] + " has a multiplier outside 0.5–1.5");
  }
  return true;
}

// Shape-preserving cubic (PCHIP, Fritsch–Carlson slopes): smooth, no overshoot, and unlike
// Monotone it follows data that goes up and down.
namespace {
struct Pchip {
  std::vector<double> x, y, m;
  Pchip(std::vector<double> xs, std::vector<double> ys) : x(std::move(xs)), y(std::move(ys)), m(x.size(), 0) {
    size_t n = x.size();
    if (n < 2) return;
    std::vector<double> h(n - 1), d(n - 1);
    for (size_t i = 0; i + 1 < n; i++) {
      h[i] = x[i + 1] - x[i];
      d[i] = (y[i + 1] - y[i]) / h[i];
    }
    m[0] = d[0];
    m[n - 1] = d[n - 2];
    for (size_t i = 1; i + 1 < n; i++) {
      if (d[i - 1] * d[i] <= 0) continue;
      double w1 = 2 * h[i] + h[i - 1], w2 = h[i] + 2 * h[i - 1];
      m[i] = (w1 + w2) / (w1 / d[i - 1] + w2 / d[i]);
    }
    if (n == 2) return;
    // End slopes may not change sign or exceed three times the secant.
    for (size_t e : {size_t(0), n - 1}) {
      double de = e == 0 ? d[0] : d[n - 2];
      if (m[e] * de <= 0) m[e] = 0;
      else if (std::fabs(m[e]) > 3 * std::fabs(de)) m[e] = 3 * de;
    }
  }
  double operator()(double v) const {
    size_t n = x.size();
    if (n == 0) return 1;
    if (v <= x[0]) return y[0];
    if (v >= x[n - 1]) return y[n - 1];
    size_t i = std::upper_bound(x.begin(), x.end(), v) - x.begin() - 1;
    double h = x[i + 1] - x[i], t = (v - x[i]) / h, t2 = t * t, t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * y[i] + (t3 - 2 * t2 + t) * h * m[i] + (-2 * t3 + 3 * t2) * y[i + 1] + (t3 - t2) * h * m[i + 1];
  }
};
}  // namespace

Lut1D pal_apply(const Lut1D &current, const PalAdjust &a, TableIndex mode) {
  Lut1D out = current;
  std::vector<double> xs;
  for (int p : a.pct) xs.push_back(pal_table_x(p, mode));
  for (int c = 0; c < 3; c++) {
    Pchip f(xs, a.mult[c]);
    for (int i = 0; i < Lut1D::kSize; i++) {
      double x = static_cast<double>(i) / (Lut1D::kSize - 1);
      out.ch[c][i] = std::clamp(current.ch[c][i] * f(x), 0.0, 1.0);
    }
  }
  return out;
}

std::array<std::vector<double>, 3> pal_table_points(const Lut1D &t, TableIndex mode) {
  std::array<std::vector<double>, 3> pts;
  for (int p : pal_levels())
    for (int c = 0; c < 3; c++) pts[c].push_back(t.eval(c, pal_table_x(p, mode)));
  return pts;
}

GateResult pal_gate(const Lut1D &next, const Lut1D &current, double max_step, bool check_bend) {
  GateResult g;
  auto fail = [&](const std::string &w) {
    g.ok = false;
    g.why = w;
    return g;
  };
  for (int c = 0; c < 3; c++) {
    const auto &n = next.ch[c], &k = current.ch[c];
    std::string ch = std::string("channel ") + "RGB"[c];
    if (n.size() != static_cast<size_t>(Lut1D::kSize) || k.size() != static_cast<size_t>(Lut1D::kSize)) return fail(ch + ": not 1024 entries");
    for (int i = 0; i < Lut1D::kSize; i++) {
      if (!std::isfinite(n[i]) || n[i] < 0 || n[i] > 1) return fail(ch + ": a value outside 0–1 at entry " + std::to_string(i));
      if (i && n[i] < n[i - 1] - 1e-7) return fail(ch + ": falls at entry " + std::to_string(i) + " (a table must rise)");
      double step = std::fabs(n[i] - k[i]);
      g.max_step = std::max(g.max_step, step);
      if (step > max_step) {
        char b[160];
        std::snprintf(b, sizeof b, "%s: entry %d moves %.3f, more than the %.2f allowed in one pass", ch.c_str(), i, step, max_step);
        return fail(b);
      }
      if (check_bend && i > 0 && i + 1 < Lut1D::kSize) {
        // A spike: bending much more sharply than the table in the TV does there.
        double bend = std::fabs(n[i + 1] - 2 * n[i] + n[i - 1]), was = std::fabs(k[i + 1] - 2 * k[i] + k[i - 1]);
        g.max_bend = std::max(g.max_bend, bend);
        if (bend > was + 0.003) return fail(ch + ": a spike at entry " + std::to_string(i));
      }
    }
  }
  g.ok = true;
  return g;
}

PalAdjust pal_propose(const std::vector<GreyLevel> &levels, const Lut1D &current, const Vec3 prim[3], const SdrTarget &t, double damp, TableIndex mode,
                      std::vector<std::string> *notes) {
  PalAdjust a;
  auto note = [&](const std::string &s) {
    if (notes) notes->push_back(s);
  };
  for (const GreyLevel &l : levels) a.pct.push_back(l.pct);
  for (auto &m : a.mult) m.assign(levels.size(), 1.0);
  const GreyLevel *white = nullptr;
  for (const GreyLevel &l : levels)
    if (l.pct == 100 && l.measured) white = &l;
  if (!white) {
    note("no white reading: nothing proposed");
    return a;
  }
  // Drive space: the primaries' matrix, scaled so the measured white is (1, 1, 1).
  Mat3 M = Mat3::columns(prim[0], prim[1], prim[2]);
  Vec3 s = M.inverse() * white->XYZ;
  Mat3 Ms = Mat3::columns(prim[0] * s.x, prim[1] * s.y, prim[2] * s.z), inv = Ms.inverse();
  double ymin = std::max(0.05, 0.002 * white->XYZ.y);
  std::array<std::vector<double>, 3> dx, ox;
  for (int c = 0; c < 3; c++) {
    dx[c].push_back(0);
    ox[c].push_back(current.eval(c, pal_table_x(0, mode)));
  }
  std::vector<bool> readable(levels.size(), false);
  for (size_t k = 0; k < levels.size(); k++) {
    const GreyLevel &l = levels[k];
    if (!l.measured || l.XYZ.y < ymin || l.pct == 0) continue;
    readable[k] = true;
    Vec3 d = inv * l.XYZ;
    for (int c = 0; c < 3; c++) {
      dx[c].push_back(d[c]);
      ox[c].push_back(current.eval(c, pal_table_x(l.pct, mode)));
    }
  }
  std::array<Monotone, 3> table_for_drive, drive_for_table;
  for (int c = 0; c < 3; c++) {
    table_for_drive[c].fit(dx[c], ox[c]);
    drive_for_table[c].fit(ox[c], dx[c]);
  }
  // The most drive each channel can give (its table at full).
  Vec3 dmax{drive_for_table[0](1.0), drive_for_table[1](1.0), drive_for_table[2](1.0)};
  bool short_white = false;
  double white_reach = 1;
  for (size_t k = 0; k < levels.size(); k++) {
    if (!readable[k]) continue;
    const GreyLevel &l = levels[k];
    Vec3 want = inv * pal_target(t, l.pct);
    // If a channel would need more than full drive, aim lower with the same colour: a grey
    // the right colour and a little dim (light fixes that) beats one that is tinted.
    double reach = 1;
    for (int c = 0; c < 3; c++)
      if (want[c] > 1e-9) reach = std::min(reach, dmax[c] / want[c]);
    if (reach < 1) {
      want = want * reach;
      if (l.pct == 100) {
        short_white = true;
        white_reach = reach;
      }
    }
    for (int c = 0; c < 3; c++) {
      double o = current.eval(c, pal_table_x(l.pct, mode));
      double o_new = std::min(table_for_drive[c](want[c]), 1.0);
      double m = o > 1e-4 ? o_new / o : 1.0;
      m = std::clamp(1 + damp * (m - 1), 0.75, 1.25);
      a.mult[c][k] = m;
    }
  }
  if (short_white) {
    char b[160];
    std::snprintf(b, sizeof b, "white can reach %.1f%% of the target at this light: raise light about %.0f%% (pal-light)", 100 * white_reach,
                  std::ceil(100 * (1 / white_reach - 1)) + 1);
    note(b);
  }
  // Levels too dark to read: half of the nearest readable level's correction; 0% stays as it is.
  for (size_t k = 0; k < levels.size(); k++) {
    if (readable[k] || levels[k].pct == 0) continue;
    size_t j = k;
    while (j < levels.size() && !readable[j]) j++;
    if (j == levels.size()) continue;
    for (int c = 0; c < 3; c++) a.mult[c][k] = 1 + 0.5 * (a.mult[c][j] - 1);
    char b[96];
    std::snprintf(b, sizeof b, "%d%% too dark to read: half of %d%%'s correction", levels[k].pct, levels[j].pct);
    note(b);
  }
  return a;
}

Lut1D pal_plateau(const Lut1D &current, double x0, double x1) {
  Lut1D out = current;
  int i0 = static_cast<int>(std::lround(x0 * (Lut1D::kSize - 1))), i1 = static_cast<int>(std::lround(x1 * (Lut1D::kSize - 1)));
  i0 = std::clamp(i0, 0, Lut1D::kSize - 1);
  i1 = std::clamp(i1, i0, Lut1D::kSize - 1);
  for (int c = 0; c < 3; c++)
    for (int i = i0; i <= i1; i++) out.ch[c][i] = current.ch[c][i0];
  return out;
}

}  // namespace cal
