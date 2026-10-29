// HDR10 (§7.4) and Dolby Vision (§7.5) calibration.
#include "hdr.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cal {

static Patch grey_patch(int c) { return {c, c, c}; }
static int pq_code(double nits) { return code_limited(pq_inverse(nits)); }

// ---- HDR10 ---------------------------------------------------------------

namespace {

struct HdrRun {
  Device &dev;
  Measurer meter;
  HdrOptions opt;
  HdrResult &res;
  Mat3 to_xyz = rgb_to_xyz(kBt2020);

  void log(const std::string &s) { res.log.push_back(s); }

  Vec3 target(Vec3 signal) const { return to_xyz * Vec3{pq_eotf(signal.x), pq_eotf(signal.y), pq_eotf(signal.z)}; }

  struct Probe {
    Patch patch;
    Vec3 signal;
    bool grey;
  };
  static Probe probe(Vec3 s, bool grey) {
    Patch p{code_limited(s.x), code_limited(s.y), code_limited(s.z)};
    return {p, {signal_limited(p.r), signal_limited(p.g), signal_limited(p.b)}, grey};
  }

  // Verification: greys up to `grey_top` nits (never the calibration levels), and the six
  // colours at three saturations, kept where they lie inside P3 (what HDR is mastered in
  // and what this panel can show).
  std::vector<Probe> verification(double grey_top) const {
    static const Mat3 panel_inverse = rgb_to_xyz(kP3D65).inverse();
    std::vector<Probe> v;
    for (double L : {0.15, 0.4, 0.8, 1.5, 4.0, 8.0, 12.0, 25.0, 40.0, 60.0, 90.0, 120.0, 180.0, 250.0, 350.0, 450.0, 550.0, 650.0})
      if (L <= grey_top) {
        double s = pq_inverse(L);
        v.push_back(probe({s, s, s}, true));
      }
    const Vec3 hues[6] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
    for (const Vec3 &h : hues)
      for (double sat : {0.25, 0.5, 0.75}) {
        Vec3 s;
        for (int c = 0; c < 3; c++) s[c] = h[c] > 0 ? 0.5 : pq_inverse(pq_eotf(0.5) * (1 - sat));
        Probe p = probe(s, false);
        Vec3 native = panel_inverse * target(p.signal);
        double sum = native.x + native.y + native.z;
        if (native.x >= -0.01 * sum && native.y >= -0.01 * sum && native.z >= -0.01 * sum) v.push_back(p);
      }
    return v;
  }

  bool score(const std::vector<Probe> &set, Stats *s) {
    *s = Stats{};
    for (const Probe &p : set) {
      Vec3 x;
      if (!meter.read(p.patch, &x, 3)) return false;
      double de = de_itp(x, target(p.signal));
      double &avg = p.grey ? s->grey_avg : s->colour_avg;
      double &mx = p.grey ? s->grey_max : s->colour_max;
      int &n = p.grey ? s->grey_n : s->colour_n;
      avg += de;
      mx = std::max(mx, de);
      n++;
    }
    if (s->grey_n) s->grey_avg /= s->grey_n;
    if (s->colour_n) s->colour_avg /= s->colour_n;
    return true;
  }

  // The panel's clip point: raise the PQ code from the 500 cd/m² level until readings stop rising.
  bool find_peak() {
    double best = 0;
    for (int code = pq_code(500); code <= 940; code += 8) {
      Vec3 x;
      if (!meter.read(grey_patch(code), &x, 3)) return false;
      if (x.y < best * 1.002) break;
      best = std::max(best, x.y);
    }
    res.peak = best;
    char b[64];
    std::snprintf(b, sizeof b, "peak %.0f cd/m2", best);
    log(b);
    return best > 0;
  }

  // Calman's matrix LUT [calman]: the P3 primaries inside the BT.2020 container (which
  // the panel can show without clipping) at 50% stimulus give the pipeline's response
  // A (XYZ per cd/m² of input); K = A⁻¹ · BT.2020 corrects colours, baked into the
  // BT.2020 3D LUT in linear light. The grey axis is left exactly as it is.
  bool matrix_lut(Lut3D *out, Mat3 *response, Mat3 *correction) {
    const double L50 = pq_eotf(0.5);
    Mat3 p3 = rgb_to_xyz(kP3D65), into2020 = to_xyz.inverse() * p3;
    Vec3 u[3], X[3];
    for (int c = 0; c < 3; c++) {
      Vec3 e;
      e[c] = 1;
      u[c] = into2020 * e * L50;  // cd/m² per BT.2020 channel
      Patch p{code_limited(pq_inverse(u[c].x)), code_limited(pq_inverse(u[c].y)), code_limited(pq_inverse(u[c].z))};
      if (!meter.read(p, &X[c], 3)) return false;
      u[c] = {pq_eotf(signal_limited(p.r)), pq_eotf(signal_limited(p.g)), pq_eotf(signal_limited(p.b))};  // what was really sent
    }
    Mat3 A = Mat3::columns(X[0], X[1], X[2]) * Mat3::columns(u[0], u[1], u[2]).inverse();
    Mat3 K = A.inverse() * to_xyz;
    *response = A;
    *correction = K;
    // On WOLED the primaries add up (the white sub-pixel is off when one channel is at
    // zero) but greys do not (the white sub-pixel carries the shared part, and the 1D LUT
    // has already corrected them). So the matrix applies in full to colours with one
    // channel at zero and fades out towards grey, by saturation s = 1 − min/max: the
    // share of the colour the white sub-pixel cannot carry. The table stays continuous.
    char b[160];
    Vec3 k1 = K * Vec3{1, 1, 1};
    std::snprintf(b, sizeof b, "matrix: K·white %.3f %.3f %.3f (faded out on greys)", k1.x, k1.y, k1.z);
    log(b);
    Lut3D lut = Lut3D::unity(33);
    const double ky[3] = {to_xyz.m[1][0], to_xyz.m[1][1], to_xyz.m[1][2]};
    for (int bb = 0; bb < 33; bb++)
      for (int g = 0; g < 33; g++)
        for (int r = 0; r < 33; r++) {
          if (r == g && g == bb) continue;
          Vec3 sig{r / 32.0, g / 32.0, bb / 32.0};
          Vec3 lin{pq_eotf(sig.x), pq_eotf(sig.y), pq_eotf(sig.z)};
          double hi = std::max({lin.x, lin.y, lin.z}), lo = std::min({lin.x, lin.y, lin.z});
          double sat = hi > 0 ? 1 - lo / hi : 0;
          Vec3 m = lin + (K * lin - lin) * sat;
          // Out of the panel's reach: towards the grey of the same luminance until no channel is negative.
          double Y = ky[0] * lin.x + ky[1] * lin.y + ky[2] * lin.z, t = 1;
          for (int c = 0; c < 3; c++)
            if (m[c] < 0) t = std::min(t, Y / (Y - m[c]));
          for (int c = 0; c < 3; c++) m[c] = std::max(0.0, Y + t * (m[c] - Y));
          lut.at(r, g, bb) = {pq_inverse(m.x), pq_inverse(m.y), pq_inverse(m.z)};
        }
    *out = lut;
    return true;
  }
};

}  // namespace

HdrResult run_hdr10(Device &dev, const HdrOptions &options) {
  HdrResult res;
  HdrRun run{dev, Measurer(dev), options, res};
  auto fail = [&](const std::string &why) {
    if (res.error.empty()) res.error = why;
    res.readings = run.meter.readings();
    dev.set_calibration_mode(false);
    return res;
  };
  run.meter.set_sentinel(grey_patch(pq_code(200)));

  // The starting point, in the viewing state.
  if (!dev.set_calibration_mode(false) || !run.meter.rebase()) return fail("meter failed");
  if (!run.score(run.verification(500), &res.baseline)) return fail("meter failed");
  run.log("baseline: " + res.baseline.str());

  // Calibration mode, bypass, peak.
  if (!dev.set_calibration_mode(true)) return fail("calibration mode refused");
  if (!dev.upload_1d(Lut1D::unity()) || !dev.upload_3d(Slot::Bt709, Lut3D::unity()) || !dev.upload_3d(Slot::Bt2020, Lut3D::unity()))
    return fail("bypass upload failed");
  if (!run.meter.rebase() || !run.find_peak()) return fail("peak not found");

  // Grey to PQ and D65, up to grey_top of the peak, scored in ΔE ITP.
  GreySpec g;
  double top = res.peak * options.grey_top;
  for (double L : {0.1, 0.25, 0.5, 1.0, 2.0, 3.5, 5.0, 7.5, 10.0, 15.0, 20.0, 30.0, 50.0, 75.0, 100.0, 150.0, 200.0, 300.0, 400.0, 500.0, 600.0})
    if (L < top * 0.97 && (g.codes.empty() || pq_code(L) > g.codes.back())) g.codes.push_back(pq_code(L));
  if (g.codes.empty() || pq_code(top) > g.codes.back()) g.codes.push_back(pq_code(top));
  g.passes = options.grey_passes;
  g.signal = signal_limited;
  g.target = [](int code) { return xyY(kD65x, kD65y, pq_eotf(signal_limited(code))); };
  g.error = [](const Vec3 &a, const Vec3 &b) { return de_itp(a, b); };
  GreyOutcome grey = solve_grey(dev, run.meter, g, Lut1D::unity(), false, &res.log);
  if (!grey.ok) return fail("grey stage failed");
  res.lut1d = grey.lut;

  // Colour: the matrix LUT, kept only if it measurably helps.
  Lut3D matrix;
  Mat3 A, K;
  if (!run.matrix_lut(&matrix, &A, &K)) return fail("meter failed");
  auto vset = run.verification(top);
  if (!run.score(vset, &res.without_matrix)) return fail("meter failed");
  run.log("without matrix: " + res.without_matrix.str());
  if (options.matrix_lut) {
    // Activation check (§6.5) on the P3 primary the matrix moves most: the change it
    // predicts (through the measured response A) must show up on the meter.
    Mat3 into2020 = run.to_xyz.inverse() * rgb_to_xyz(kP3D65);
    HdrRun::Probe probe = HdrRun::probe({0, 0, 0}, false);
    double predicted = 0;
    for (int c = 0; c < 3; c++) {
      Vec3 e;
      e[c] = 1;
      Vec3 lin = into2020 * e * pq_eotf(0.5);
      HdrRun::Probe p = HdrRun::probe({pq_inverse(lin.x), pq_inverse(lin.y), pq_inverse(lin.z)}, false);
      Vec3 sent{pq_eotf(p.signal.x), pq_eotf(p.signal.y), pq_eotf(p.signal.z)};
      double d = de_itp(A * sent, A * (K * sent));
      if (d > predicted) predicted = d, probe = p;
    }
    Vec3 before, after;
    if (!run.meter.read(probe.patch, &before, 3)) return fail("meter failed");
    if (!dev.upload_3d(Slot::Bt2020, matrix) || !run.meter.rebase()) return fail("3D LUT upload failed");
    if (!run.meter.read(probe.patch, &after, 3)) return fail("meter failed");
    {
      char b[200];
      Vec3 t = run.target(probe.signal);
      std::snprintf(b, sizeof b, "activation probe %d %d %d: predicted %.2f, moved %.2f; before->target %.2f, after->target %.2f", probe.patch.r, probe.patch.g,
                    probe.patch.b, predicted, de_itp(after, before), de_itp(before, t), de_itp(after, t));
      run.log(b);
    }
    if (predicted >= 1 && de_itp(after, before) < 0.5 * predicted) {
      dev.upload_3d(Slot::Bt2020, Lut3D::unity());
      return fail("the TV accepted the 3D LUT but the picture did not change (firmware ignored it)");
    }
    if (!run.score(vset, &res.with_matrix)) return fail("meter failed");
    run.log("with matrix: " + res.with_matrix.str());
    res.matrix_kept = res.with_matrix.colour_avg < res.without_matrix.colour_avg - 0.2 &&
                      res.with_matrix.grey_avg <= res.without_matrix.grey_avg + 0.3;
    if (res.matrix_kept) res.lut3d = matrix;
    else if (!dev.upload_3d(Slot::Bt2020, Lut3D::unity()) || !run.meter.rebase()) return fail("3D LUT upload failed");
    run.log(res.matrix_kept ? "matrix LUT kept" : "matrix LUT did not help: removed");
  }

  // Tone mapping from the measured peak (bscpylgtv's worked roll-offs).
  int tm[7] = {static_cast<int>(std::lround(std::clamp(res.peak, 100.0, 4000.0))), 1000, 75, 4000, 60, 10000, 50};
  if (!dev.set_tonemap(tm)) return fail("tone mapping refused");
  std::copy(tm, tm + 7, res.tonemap);

  // The viewing state: greys up to the tone-mapping knee, colours, and highlights that keep rising.
  if (!dev.set_calibration_mode(false) || !run.meter.rebase()) return fail("leaving calibration mode failed");
  if (!run.score(run.verification(res.peak * 0.7), &res.final)) return fail("meter failed");
  run.log("final: " + res.final.str());
  double prev = 0;
  res.highlights_monotonic = true;
  for (double L : {600.0, 700.0, 800.0, 900.0, 1000.0}) {
    Vec3 x;
    if (!run.meter.read(grey_patch(pq_code(L)), &x, 3)) return fail("meter failed");
    if (x.y <= prev * 1.003) res.highlights_monotonic = false;
    prev = x.y;
  }
  res.readings = run.meter.readings();
  if (res.final.worse_than(res.baseline)) {
    res.error = "the result was worse than the starting point";
    return res;
  }
  res.ok = true;
  return res;
}

// ---- Dolby Vision --------------------------------------------------------

std::string dolby_config(int picture_mode, double tmax, double tmin, double gamma, const double p[6]) {
  char b[512];
  std::snprintf(b, sizeof b,
                "# Dolby Vision User Display Configuration File\r\n"
                "# Generated by Bloob\r\n"
                "# Display: LG OLED\r\n"
                "# DM Version: \r\n"
                "[PictureMode = %d]\r\n"
                "Tmax = %.4f\r\n"
                "Tmin = %.4f\r\n"
                "Tgamma = %.1f\r\n"
                "TPrimaries = %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f\r\n",
                picture_mode, tmax, std::max(tmin, 0.0001), gamma, p[0], p[1], p[2], p[3], p[4], p[5], kD65x, kD65y);
  return b;
}

DolbyResult run_dolby(Device &dev, const DolbyOptions &options) {
  DolbyResult res;
  Measurer meter(dev);
  auto log = [&](const std::string &s) { res.log.push_back(s); };
  auto fail = [&](const std::string &why) {
    if (res.error.empty()) res.error = why;
    res.readings = meter.readings();
    dev.set_calibration_mode(false);
    return res;
  };
  meter.set_sentinel(grey_patch(1023));
  // Factory data, not an upload: an upload would disable the configuration pop-up.
  if (!dev.set_calibration_mode(true) || !dev.reset_factory() || !meter.rebase()) return fail("calibration mode refused");

  // Pass-through check (U7): a full-range ramp must follow a power curve between black and peak.
  std::vector<double> lx, ly;
  Vec3 w, k;
  if (!meter.read(grey_patch(1023), &w, 3) || !meter.read(grey_patch(0), &k, 4)) return fail("meter failed");
  for (int i = 2; i <= 18; i++) {
    double v = i / 20.0;
    Vec3 x;
    if (!meter.read(grey_patch(static_cast<int>(std::lround(v * 1023))), &x, 3)) return fail("meter failed");
    lx.push_back(std::log(v));
    ly.push_back(std::log(std::max((x.y - k.y) / (w.y - k.y), 1e-6)));
  }
  double mx = 0, my = 0;
  for (size_t i = 0; i < lx.size(); i++) mx += lx[i], my += ly[i];
  mx /= lx.size();
  my /= ly.size();
  double sxx = 0, sxy = 0, syy = 0;
  for (size_t i = 0; i < lx.size(); i++) {
    sxx += (lx[i] - mx) * (lx[i] - mx);
    sxy += (lx[i] - mx) * (ly[i] - my);
    syy += (ly[i] - my) * (ly[i] - my);
  }
  double slope = sxy / sxx, r2 = sxy * sxy / (sxx * syy);
  res.passthrough = r2 > 0.998 && slope > 1.8 && slope < 2.7;
  char b[160];
  std::snprintf(b, sizeof b, "pass-through: exponent %.2f, fit %.4f -> %s", slope, r2, res.passthrough ? "yes" : "no");
  log(b);
  if (!res.passthrough) return fail("Dolby Vision patches do not reach the panel directly, so the grey fix was skipped");

  // White, black, red, green and blue on the factory data, for the configuration. The
  // factory table already balances white close to D65, so its white is what the grey fix
  // will be able to reach: Tmax is set from it, just below.
  Vec3 xr, xg, xb;
  if (!meter.read({1023, 0, 0}, &xr, 3) || !meter.read({0, 1023, 0}, &xg, 3) || !meter.read({0, 0, 1023}, &xb, 3)) return fail("meter failed");
  double prim[6];
  chromaticity(xr, &prim[0], &prim[1]);
  chromaticity(xg, &prim[2], &prim[3]);
  chromaticity(xb, &prim[4], &prim[5]);
  res.tmin = std::max(k.y, 0.0001);
  res.tmax = w.y * 0.99;
  std::snprintf(b, sizeof b, "Tmax %.1f (factory white %.1f), Tmin %.4f", res.tmax, w.y, res.tmin);
  log(b);

  GreySpec g;
  for (int i = 1; i <= 32; i++) g.codes.push_back(static_cast<int>(std::lround(std::pow(i / 32.0, 1.5) * 1023)));
  g.code_lo = 0;
  g.code_hi = 1023;
  g.passes = options.grey_passes;
  g.signal = [](int code) { return code / 1023.0; };
  double tmax = res.tmax;
  g.target = [&](int code) { return xyY(kD65x, kD65y, res.tmin + (tmax - res.tmin) * std::pow(code / 1023.0, 2.2)); };
  Vec3 white = xyY(kD65x, kD65y, tmax);
  g.error = [&](const Vec3 &a, const Vec3 &c) { return de2000_xyz(a, c, white); };

  // The configuration goes in before any 1D LUT: afterwards the TV no longer offers its pop-up.
  res.config = dolby_config(2, res.tmax, res.tmin, 2.2, prim);
  dev.set_calibration_mode(false);
  res.config_loaded = dev.load_dolby_config(res.config);
  dev.set_calibration_mode(true);
  log(res.config_loaded ? "configuration loaded from USB" : "no USB stick or pop-up: configuration skipped");

  // Baseline against the same target, then the grey fix.
  Stats base;
  auto verify = [&](Stats *s) {
    *s = Stats{};
    for (int i = 0; i < 21; i++) {
      int code = static_cast<int>(std::lround((i + 0.5) / 21 * 1023));
      Vec3 x;
      if (!meter.read(grey_patch(code), &x, 3)) return false;
      double de = de2000_xyz(x, g.target(code), white);
      s->grey_avg += de;
      s->grey_max = std::max(s->grey_max, de);
      s->grey_n++;
    }
    s->grey_avg /= s->grey_n;
    return true;
  };
  if (!verify(&res.baseline)) return fail("meter failed");
  std::vector<Vec3> dark_before;
  for (int code : {10, 20, 30, 51}) {
    Vec3 x;
    if (!meter.read(grey_patch(code), &x, 4)) return fail("meter failed");
    dark_before.push_back(x);
  }
  if (!dev.upload_1d(Lut1D::unity()) || !meter.rebase()) return fail("1D LUT upload failed");
  GreyOutcome grey = solve_grey(dev, meter, g, Lut1D::unity(), false, &res.log);
  if (!grey.ok) return fail("grey stage failed");
  res.lut1d = grey.lut;

  // Black must not rise; the peak must match the configuration.
  // Black must not rise above both where it was and where it should be (rising towards
  // the target is the fix; rising past it is the raised black bscpylgtv warns about).
  res.black_ok = true;
  int i = 0;
  for (int code : {10, 20, 30, 51}) {
    Vec3 x;
    if (!meter.read(grey_patch(code), &x, 4)) return fail("meter failed");
    double limit = std::max(dark_before[i++].y * 1.05, g.target(code).y * 1.1) + 0.002;
    if (x.y > limit) res.black_ok = false;
  }
  Vec3 peak;
  if (!meter.read(grey_patch(1023), &peak, 3)) return fail("meter failed");
  res.peak_after = peak.y;
  res.peak_matches = std::fabs(peak.y - res.tmax) / res.tmax <= 0.03;
  if (!dev.set_calibration_mode(false) || !meter.rebase() || !verify(&res.final)) return fail("meter failed");
  log("final: " + res.final.str());
  res.readings = meter.readings();
  res.ok = res.final.grey_avg <= res.baseline.grey_avg + 0.05;
  if (!res.ok) res.error = "the result was worse than the starting point";
  return res;
}

}  // namespace cal
