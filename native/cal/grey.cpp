#include "grey.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "model.h"

namespace cal {

GreyOutcome solve_grey(Device &dev, Measurer &meter, const GreySpec &spec, const Lut1D &start, bool stop_if_unreachable,
                       std::vector<std::string> *log) {
  GreyOutcome out;
  const int n = static_cast<int>(spec.codes.size());
  if (n < 3) return out;
  std::vector<double> lv(n);
  for (int k = 0; k < n; k++) lv[k] = spec.signal(spec.codes[k]);
  auto outputs = [&](const Lut1D &l, int k) { return Vec3{l.eval(0, lv[k]), l.eval(1, lv[k]), l.eval(2, lv[k])}; };
  std::vector<Mat3> J(n);
  std::vector<bool> haveJ(n, false);
  std::vector<Vec3> prev_o(n), prev_m(n), j_at(n);
  Lut1D lut = start, best = start;
  double best_score = 1e300;
  int stale = 0;
  for (int pass = 0; pass < spec.passes; pass++) {
    std::vector<Vec3> m(n);
    for (int k = 0; k < n; k++) {
      int c = spec.codes[k];
      if (!meter.read({c, c, c}, &m[k], spec.grey_reads)) return out;
    }
    // Score this table: the average, with the worst level counting too.
    double avg = 0, worst = 0;
    for (int k = 0; k < n; k++) {
      double de = spec.error(m[k], spec.target(spec.codes[k]));
      avg += de;
      worst = std::max(worst, de);
    }
    double score = avg / n + 0.25 * worst;
    // Broyden: what this pass measured corrects each level's sensitivity estimate.
    // Only steps well above the meter's noise teach anything; half weight, for stability.
    if (pass > 0)
      for (int k = 0; k < n; k++) {
        Vec3 d_o = outputs(lut, k) - prev_o[k], d_m = m[k] - prev_m[k];
        double dd = d_o.x * d_o.x + d_o.y * d_o.y + d_o.z * d_o.z;
        if (dd < 1e-4 || m[k].y < 1.0) continue;
        Vec3 miss = d_m - J[k] * d_o;
        for (int r = 0; r < 3; r++)
          for (int c = 0; c < 3; c++) J[k].m[r][c] += 0.5 * miss[r] * d_o[c] / dd;
      }
    for (int k = 0; k < n; k++) {
      prev_o[k] = outputs(lut, k);
      prev_m[k] = m[k];
    }
    if (log) {
      char b[96];
      std::snprintf(b, sizeof b, "grey pass %d: %.3f", pass, score);
      log->push_back(b);
      if (g_log_hook) g_log_hook(b);
    }
    if (score < best_score - 0.01) {
      best_score = score;
      best = lut;
      stale = 0;
    } else if (pass > 0 && ++stale >= 3) {
      break;  // stopped improving
    }

    // Sensitivities go stale as the outputs move (the panel's slope changes with
    // drive), so they are measured again when a level has moved more than 3%.
    bool stale_j = pass == 0;
    for (int k = 0; k < n && !stale_j; k++) {
      Vec3 o = outputs(lut, k);
      for (int c = 0; c < 3; c++)
        if (haveJ[k] && m[k].y >= 1.0 && std::fabs(o[c] - j_at[k][c]) > 0.03 * std::max(j_at[k][c], 0.05)) stale_j = true;
    }
    if (stale_j) {
      // Sensitivity of XYZ to each 1D output, from grey ± `step` codes on one channel.
      // Central differences, because WOLED greys have a kink: lowering one channel
      // also dims the white sub-pixel, raising it does not.
      std::fill(haveJ.begin(), haveJ.end(), false);
      for (int k = 0; k < n; k++) {
        j_at[k] = outputs(lut, k);
        int code = spec.codes[k], s = spec.step;
        if (m[k].y < 1.0 || code + s > spec.code_hi || code - s < spec.code_lo) continue;
        if (spec.jac_every > 1 && k % spec.jac_every != 0 && k != n - 2) continue;
        double dv = spec.signal(code + s) - spec.signal(code - s);
        for (int c = 0; c < 3; c++) {
          Vec3 up, down;
          Patch pu{code, code, code}, pd{code, code, code};
          (c == 0 ? pu.r : c == 1 ? pu.g : pu.b) += s;
          (c == 0 ? pd.r : c == 1 ? pd.g : pd.b) -= s;
          if (!meter.read(pu, &up, spec.jac_reads) || !meter.read(pd, &down, spec.jac_reads)) return out;
          double dout = lut.eval(c, spec.signal(code + s)) - lut.eval(c, spec.signal(code - s));
          if (std::fabs(dout) < 1e-4 * dv) dout = 1e-4 * dv;
          Vec3 col = (up - down) / dout;
          for (int r = 0; r < 3; r++) J[k].m[r][c] = col[r];
        }
        haveJ[k] = true;
      }
      // The top cannot step up: it borrows from the level below, scaled by slope (∝ Y/v).
      for (int k = n - 1; k >= 0 && !haveJ[k]; k--) {
        int below = k - 1;
        while (below >= 0 && !haveJ[below]) below--;
        if (below < 0 || m[k].y < 1.0) break;
        double f = m[k].y / std::max(m[below].y, 1e-6) * lv[below] / std::max(lv[k], 1e-6);
        for (int r = 0; r < 3; r++)
          for (int c = 0; c < 3; c++) J[k].m[r][c] = J[below].m[r][c] * f;
        haveJ[k] = true;
      }
      // Levels skipped by jac_every borrow from the nearest measured level below, scaled by slope (∝ Y/v).
      for (int k = 0; k < n; k++) {
        if (haveJ[k] || m[k].y < 1.0) continue;
        int b = k - 1;
        while (b >= 0 && !haveJ[b]) b--;
        if (b < 0) continue;
        double f = m[k].y / std::max(m[b].y, 1e-6) * lv[b] / std::max(lv[k], 1e-6);
        for (int r = 0; r < 3; r++)
          for (int c = 0; c < 3; c++) J[k].m[r][c] = J[b].m[r][c] * f;
        haveJ[k] = true;
      }
      // Near black: the nearest measured sensitivity, scaled by the local slope of the grey ramp.
      int ref = -1;
      for (int k = 0; k < n && ref < 0; k++)
        if (haveJ[k]) ref = k;
      if (ref < 0) return out;
      auto slope = [&](int k) {
        int a = std::max(k - 1, 0), c = std::min(k + 1, n - 1);
        return std::max((m[c].y - m[a].y) / (lv[c] - lv[a]), 1e-6);
      };
      for (int k = 0; k < ref; k++) {
        double f = slope(k) / slope(ref);
        for (int r = 0; r < 3; r++)
          for (int c = 0; c < 3; c++) J[k].m[r][c] = J[ref].m[r][c] * f;
        haveJ[k] = true;
      }
    }

    // Can the top target be reached at all? (A channel would have to pass full drive.)
    if (pass == 0) {
      int top = n - 1;
      Mat3 Ji = J[top].inverse();
      Vec3 base = outputs(lut, top) - Ji * m[top], dir = Ji * spec.target(spec.codes[top]);
      double alpha = 1;
      for (int c = 0; c < 3; c++)
        if (dir[c] > 1e-12) alpha = std::min(alpha, (1 - base[c]) / dir[c]);
      out.reach = alpha;
      if (alpha < 1 && stop_if_unreachable) {
        out.ok = true;
        out.lut = start;
        out.score = score;
        return out;
      }
    }

    // Newton step per level, then a monotone curve through the new outputs.
    std::vector<Vec3> delta(n);
    for (int k = 0; k < n; k++) {
      double damp = (m[k].y > 5 ? 0.9 : 0.6) * (pass < 3 ? 1.0 : 0.7);
      delta[k] = J[k].inverse() * (spec.target(spec.codes[k]) - m[k]) * damp;
    }
    // No table value moves more than 0.1 in one pass: a sensitivity that read as almost
    // nothing (a level that did not respond) must not turn into a wild step.
    for (auto &d : delta)
      for (int c = 0; c < 3; c++) d[c] = std::clamp(d[c], -0.1, 0.1);
    // After the first big steps the remaining error is mostly smooth in level and the
    // meter's noise is not: smooth each level's correction with its neighbours.
    if (pass >= 2) {
      std::vector<Vec3> sm(n);
      for (int k = 0; k < n; k++) {
        int a = std::max(k - 1, 0), c = std::min(k + 1, n - 1);
        sm[k] = delta[a] * 0.25 + delta[k] * 0.5 + delta[c] * 0.25;
      }
      delta = sm;
    }
    std::array<std::vector<double>, 3> xs, ys;
    for (int c = 0; c < 3; c++) {
      xs[c] = {0};
      ys[c] = {lut.eval(c, 0)};
    }
    for (int k = 0; k < n; k++) {
      Vec3 o = outputs(lut, k);
      for (int c = 0; c < 3; c++) {
        xs[c].push_back(lv[k]);
        ys[c].push_back(std::clamp(o[c] + delta[k][c], 0.0, 1.0));
      }
    }
    Lut1D next;
    for (int c = 0; c < 3; c++) {
      Monotone f;
      f.fit(xs[c], ys[c]);
      next.ch[c].resize(Lut1D::kSize);
      for (int i = 0; i < Lut1D::kSize; i++) next.ch[c][i] = std::clamp(f(i / 1023.0), 0.0, 1.0);
    }
    lut = next;
    if (!dev.upload_1d(lut) || !meter.rebase()) return out;
  }
  if (!dev.upload_1d(best) || !meter.rebase()) return out;
  out.ok = true;
  out.lut = best;
  out.score = best_score;
  return out;
}

}  // namespace cal
