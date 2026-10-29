// SDR calibration (CALIBRATION-PLAN.md §7.2), against any Device.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>

#include "procedure.h"

namespace cal {

// ---- Targets and statistics ---------------------------------------------

double SdrTarget::eotf(double v) const { return bt1886 ? cal::bt1886(v, Lw, Lb) : power_eotf(v, gamma, Lw, Lb); }

Vec3 SdrTarget::xyz(Vec3 s) const {
  static thread_local Primaries cached{};
  static thread_local Mat3 m;
  if (std::memcmp(&cached, &primaries, sizeof cached) != 0) {
    cached = primaries;
    m = rgb_to_xyz(primaries);
  }
  return m * Vec3{eotf(s.x), eotf(s.y), eotf(s.z)};
}

std::string Stats::str() const {
  char b[160];
  std::snprintf(b, sizeof b, "grey %.2f avg / %.2f max (%d), colour %.2f avg / %.2f max (%d)", grey_avg, grey_max, grey_n, colour_avg,
                colour_max, colour_n);
  return b;
}

bool Stats::worse_than(const Stats &o) const { return grey_avg > o.grey_avg + 0.05 || colour_avg > o.colour_avg + 0.05; }

// ---- Patch sets (§5) -----------------------------------------------------

struct Probe {
  Patch patch;
  Vec3 signal;
  bool grey;
};

static Probe probe_of(Vec3 s, bool grey) {
  Patch p{code_limited(s.x), code_limited(s.y), code_limited(s.z)};
  return {p, {signal_limited(p.r), signal_limited(p.g), signal_limited(p.b)}, grey};
}

// The ColorChecker's 24 colours (commonly published sRGB values), used as video signals.
static const int kChecker[24][3] = {{115, 82, 68},   {194, 150, 130}, {98, 122, 157},  {87, 108, 67},   {133, 128, 177}, {103, 189, 170},
                                    {214, 126, 44},  {80, 91, 166},   {193, 90, 99},   {94, 60, 108},   {157, 188, 64},  {224, 163, 46},
                                    {56, 61, 150},   {70, 148, 73},   {175, 54, 60},   {231, 199, 31},  {187, 86, 149},  {8, 133, 161},
                                    {243, 243, 242}, {200, 200, 200}, {160, 160, 160}, {122, 122, 121}, {85, 85, 85},    {52, 52, 52}};

// Verification set. `variant` 0 steers the closed loop; 1 is the final, never used anywhere else.
static std::vector<Probe> verification_set(int variant, unsigned seed) {
  std::vector<Probe> v;
  double offset = variant == 0 ? 0.5 : 0.25;
  for (int i = 0; i < 21; i++) {
    double g = (i + offset) / 21;
    v.push_back(probe_of({g, g, g}, true));
  }
  v.push_back(probe_of({1, 1, 1}, true));
  for (auto &c : kChecker) {
    Vec3 s{c[0] / 255.0, c[1] / 255.0, c[2] / 255.0};
    bool grey = c[0] == c[1] && c[1] == c[2];
    v.push_back(probe_of(s, grey || (std::fabs(s.x - s.y) < 0.01 && std::fabs(s.y - s.z) < 0.01)));
  }
  double stim = variant == 0 ? 0.75 : 0.6;
  const Vec3 hues[6] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
  for (const Vec3 &h : hues)
    for (double sat : {0.25, 0.5, 0.75, 1.0}) {
      Vec3 s;
      for (int c = 0; c < 3; c++) s[c] = stim * (h[c] > 0 ? 1 : 1 - sat);
      v.push_back(probe_of(s, false));
    }
  std::mt19937 rng(seed + 100 * variant);
  std::uniform_real_distribution<double> u(0.05, 0.95);
  for (int i = 0; i < (variant == 0 ? 10 : 20); i++) v.push_back(probe_of({u(rng), u(rng), u(rng)}, false));
  return v;
}

static std::vector<Probe> characterisation_set(int grid, unsigned seed) {
  std::vector<Probe> v;
  for (int c = 0; c < 3; c++)
    for (int i = 1; i <= 32; i++) {
      Vec3 s;
      s[c] = i / 32.0;
      v.push_back(probe_of(s, false));
    }
  v.push_back(probe_of({0, 0, 0}, true));
  for (int b = 0; b < grid; b++)
    for (int g = 0; g < grid; g++)
      for (int r = 0; r < grid; r++) {
        Vec3 s{r / double(grid - 1), g / double(grid - 1), b / double(grid - 1)};
        v.push_back(probe_of(s, r == g && g == b));
      }
  const Vec3 secondaries[3] = {{0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
  for (const Vec3 &h : secondaries)
    for (int i = 1; i <= 12; i++) v.push_back(probe_of(h * (i / 12.0), false));
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> g(0.03, 1.0), d(-0.06, 0.06);
  for (int i = 0; i < 100; i++) {
    double l = g(rng);
    v.push_back(probe_of({std::clamp(l + d(rng), 0.0, 1.0), std::clamp(l + d(rng), 0.0, 1.0), std::clamp(l + d(rng), 0.0, 1.0)}, false));
  }
  return v;
}

// ---- Steps ---------------------------------------------------------------

struct Run {
  Device &dev;
  Measurer meter;
  SdrOptions opt;
  SdrResult &res;
  Lut1D lut1d = Lut1D::unity();
  Lut3D lut3d = Lut3D::unity();
  int raises_ = 0;

  void log(const std::string &s) {
    res.log.push_back(s);
    if (g_log_hook) g_log_hook(s);
  }

  bool measure_set(const std::vector<Probe> &set, std::vector<Vec3> *out, int min_reads = 2) {
    out->resize(set.size());
    for (size_t i = 0; i < set.size(); i++)
      if (!meter.read(set[i].patch, &(*out)[i], min_reads)) return false;
    return true;
  }

  Stats score(const std::vector<Probe> &set, const std::vector<Vec3> &xyz) {
    Stats s;
    Vec3 white = opt.target.white();
    for (size_t i = 0; i < set.size(); i++) {
      double de = de2000_xyz(xyz[i], opt.target.xyz(set[i].signal), white);
      if (set[i].grey) {
        s.grey_avg += de;
        s.grey_max = std::max(s.grey_max, de);
        s.grey_n++;
      } else {
        s.colour_avg += de;
        s.colour_max = std::max(s.colour_max, de);
        s.colour_n++;
      }
    }
    if (s.grey_n) s.grey_avg /= s.grey_n;
    if (s.colour_n) s.colour_avg /= s.colour_n;
    return s;
  }

  bool verify(int variant, Stats *s) {
    auto set = verification_set(variant, opt.seed);
    std::vector<Vec3> xyz;
    if (!measure_set(set, &xyz, 3)) return false;
    *s = score(set, xyz);
    return true;
  }

  bool white_y(double *y) {
    Vec3 w;
    if (!meter.read({940, 940, 940}, &w)) return false;
    *y = w.y;
    return true;
  }

  // The smallest OLED light at which white reaches the target (§7.2 step 5). Light
  // moves in steps of a few per cent, so the grey stage trims white to the exact
  // target from there; `factor` asks for headroom (the grey stage's reachable white).
  bool search_light(double factor = 1) {
    int lo = 0, hi = 100;
    double need = opt.target.Lw / factor, y_best = 0;
    while (lo < hi) {
      int mid = (lo + hi) / 2;
      if (!dev.set_light(mid)) return false;
      double y;
      if (!meter.rebase() || !white_y(&y)) return false;
      if (y >= need) hi = mid, y_best = y;
      else lo = mid + 1;
    }
    if (!dev.set_light(lo) || !meter.rebase()) return false;
    res.light = lo;
    char b[96];
    std::snprintf(b, sizeof b, "light %d (white %.1f for a target of %.1f)", lo, y_best, need);
    log(b);
    return true;
  }

  // Pipeline order (C2.3): a 3D LUT that swaps red and green and a 1D LUT that blanks red.
  bool detect_order(bool *lut1d_first) {
    Lut3D swap = Lut3D::unity();
    for (auto &n : swap.node) std::swap(n.x, n.y);
    Lut1D blank = Lut1D::unity();
    for (double &v : blank.ch[0]) v = 0;
    if (!dev.upload_3d(Slot::Bt709, swap) || !dev.upload_1d(blank)) return false;
    Vec3 x;
    if (!meter.read(probe_of({0.75, 0, 0}, false).patch, &x)) return false;
    *lut1d_first = x.y < 0.5;  // red blanked before the swap: black; otherwise the swapped green shows
    return dev.upload_3d(Slot::Bt709, Lut3D::unity()) && dev.upload_1d(Lut1D::unity());
  }

  // ---- Grey (§6.3) ----
  // The 1D LUT puts every grey on D65 and the target curve. If the panel cannot
  // reach the target white at D65 at this light, light goes up (only ever up, by at
  // least one step, so it cannot oscillate) and the grey stage starts again.
  bool grey_stage() {
    GreySpec g;
    for (int k = 1; k <= 32; k++) g.codes.push_back(code_limited(std::pow(k / 32.0, 1.5)));
    g.passes = opt.grey_passes;
    g.jac_every = opt.jac_every;
    g.jac_reads = opt.jac_reads;
    g.grey_reads = opt.grey_reads;
    g.signal = signal_limited;
    g.target = [this](int code) { return xyY(kD65x, kD65y, opt.target.eotf(signal_limited(code))); };
    Vec3 white = opt.target.white();
    g.error = [white](const Vec3 &a, const Vec3 &b) { return de2000_xyz(a, b, white); };
    for (;;) {
      GreyOutcome r = solve_grey(dev, meter, g, lut1d, raises_ < 3, &res.log);
      if (!r.ok) return false;
      if (r.reach < 1 && raises_ < 3) {
        raises_++;
        int light = std::min(100, std::max(dev.light() + 1, static_cast<int>(std::ceil(dev.light() / (r.reach * 0.985)))));
        char b[96];
        std::snprintf(b, sizeof b, "D65 white reachable only at %.1f%%: light %d -> %d", r.reach * 100, dev.light(), light);
        log(b);
        if (light == dev.light() || !dev.set_light(light) || !dev.upload_1d(Lut1D::unity()) || !meter.rebase()) return false;
        res.light = light;
        lut1d = Lut1D::unity();
        continue;
      }
      lut1d = r.lut;
      return true;
    }
  }

  // ---- Colour (§6.4) ----
  // Out of reach (§6.4): keep hue; give up lightness only if even the neutral of
  // that lightness is out of reach, then give up chroma until the colour is
  // reachable. If nothing works the node stays uncorrected.
  Vec3 gamut_map(const PanelModel &model, const Vec3 &target, const Vec3 &signal, Vec3 x) const {
    Vec3 white = opt.target.white();
    auto from_lab = [&](Vec3 l) {
      double fy = (l.x + 16) / 116, fx = fy + l.y / 500, fz = fy - l.z / 200;
      auto inv = [](double f) { return f * f * f > 216.0 / 24389 ? f * f * f : (116 * f - 16) / (24389.0 / 27); };
      return Vec3{inv(fx) * white.x, inv(fy) * white.y, inv(fz) * white.z};
    };
    auto reach = [&](const Vec3 &t, Vec3 *out) {
      Vec3 tries[3] = {*out, signal, Vec3{0.5, 0.5, 0.5}};
      double best = 1e300;
      for (Vec3 y : tries) {
        double e = model.invert(t, white, &y);
        if (e < best) best = e, *out = y;
        if (e < 0.3) return true;
      }
      return false;
    };
    Vec3 l = lab(target, white);
    // Lightness: the brightest neutral at or below the target's that is reachable.
    double Lmax = l.x;
    Vec3 y = x;
    if (!reach(from_lab({l.x, 0, 0}), &y)) {
      double lo = 0, hi = l.x;
      for (int it = 0; it < 12; it++) {
        double mid = 0.5 * (lo + hi);
        Vec3 z = y;
        if (reach(from_lab({mid, 0, 0}), &z)) lo = mid;
        else hi = mid;
      }
      Lmax = lo;
    }
    Vec3 best = signal;
    double lo = 0, hi = 1;
    bool any = false;
    for (int it = 0; it < 12; it++) {
      double mid = 0.5 * (lo + hi);
      Vec3 z = x;
      if (reach(from_lab({Lmax, l.y * mid, l.z * mid}), &z)) lo = mid, best = z, any = true;
      else hi = mid;
    }
    return any ? best : signal;
  }

  Lut3D solve_colour(const PanelModel &model, bool grey_identity) {
    Lut3D out = Lut3D::unity(33);
    Vec3 white = opt.target.white();
    for (int b = 0; b < 33; b++)
      for (int g = 0; g < 33; g++) {
        Vec3 start{0, g / 32.0, b / 32.0};
        for (int r = 0; r < 33; r++) {
          Vec3 s{r / 32.0, g / 32.0, b / 32.0};
          if (grey_identity && r == g && g == b) {
            out.at(r, g, b) = s;
            start = s;
            continue;
          }
          Vec3 target = opt.target.xyz(s);
          Vec3 x = r ? start : s;
          double e = model.invert(target, white, &x);
          if (e > 0.5) x = gamut_map(model, target, s, x);
          out.at(r, g, b) = x;
          start = x;
        }
      }
    return out;
  }

  Stats predict(const PanelModel &model, const Lut3D &lut, int variant) {
    auto set = verification_set(variant, opt.seed);
    std::vector<Vec3> xyz;
    for (const Probe &p : set) xyz.push_back(model.predict(lut.eval(p.signal)));
    return score(set, xyz);
  }

  // Activation check (§6.5): the change the model predicts must show up on the meter.
  bool activated(const PanelModel &model, const Lut3D &before, const Lut3D &after, const std::vector<Vec3> &measured_before) {
    const Probe probes[2] = {probe_of({0.75, 0.1, 0.1}, false), probe_of({0.5, 0.5, 0.5}, true)};
    Vec3 white = opt.target.white();
    for (int i = 0; i < 2; i++) {
      Vec3 pa = model.predict(after.eval(probes[i].signal)), pb = model.predict(before.eval(probes[i].signal));
      double predicted = de2000_xyz(pa, pb, white);
      if (predicted < 1) continue;
      Vec3 now;
      if (!meter.read(probes[i].patch, &now)) return false;
      double moved = de2000_xyz(now, measured_before[i], white);
      if (moved < 0.5 * predicted) return false;
    }
    return true;
  }

  // If the model cannot reach the target white at D65 (the 3D-only route, where no
  // grey stage has balanced white yet), raise light by the shortfall. True if it did.
  bool raise_light_for(const PanelModel &model) {
    Vec3 white = opt.target.white(), x{1, 1, 1};
    double lo = 0.3, hi = 1;
    if (model.invert(white, white, &x) < 0.2) return false;
    for (int it = 0; it < 14; it++) {
      double mid = 0.5 * (lo + hi);
      Vec3 y{1, 1, 1};
      if (model.invert(white * mid, white * mid, &y) < 0.2) lo = mid;
      else hi = mid;
    }
    int light = std::min(100, static_cast<int>(std::ceil(dev.light() / (lo * 0.995))));
    if (light <= dev.light() || raised_) return false;
    raised_ = true;
    char b[96];
    std::snprintf(b, sizeof b, "white reachable only at %.0f%%: light %d -> %d", lo * 100, dev.light(), light);
    log(b);
    dev.set_light(light);
    res.light = light;
    return meter.rebase();
  }
  bool raised_ = false;

  bool colour_stage(bool grey_identity) {
    auto set = characterisation_set(opt.grid, opt.seed);
    std::vector<Vec3> xyz;
    if (!measure_set(set, &xyz)) return false;
    std::vector<Sample> samples;
    for (size_t i = 0; i < set.size(); i++) samples.push_back({lut3d.eval(set[i].signal), xyz[i], 1});
    PanelModel model;
    std::string err;
    if (!model.fit(samples, opt.lattice, &err)) {
      res.error = err;
      return false;
    }
    res.model_holdout = model.holdout_de();
    if (!grey_identity && raise_light_for(model)) return colour_stage(grey_identity);  // characterise again at the new light
    char b[128];
    std::snprintf(b, sizeof b, "model: %zu samples, held-out %.2f dE, lambda %g", samples.size(), model.holdout_de(), model.lambda());
    log(b);

    Stats best_v;
    bool have_best = false;
    Lut3D best = lut3d;
    std::mt19937 rng(opt.seed + 5);
    for (int pass = 0; pass < opt.colour_passes; pass++) {
      Lut3D next = solve_colour(model, grey_identity);
      Stats now = predict(model, lut3d, 0), then = predict(model, next, 0);
      std::snprintf(b, sizeof b, "colour pass %d predicted: %s", pass, then.str().c_str());
      log(b);
      if (then.worse_than(now) && pass > 0) break;
      // Measure the probes before the upload, for the activation check.
      std::vector<Vec3> before(2);
      const Probe probes[2] = {probe_of({0.75, 0.1, 0.1}, false), probe_of({0.5, 0.5, 0.5}, true)};
      for (int i = 0; i < 2; i++)
        if (!meter.read(probes[i].patch, &before[i])) return false;
      Lut3D previous = lut3d;
      if (!dev.upload_3d(Slot::Bt709, next) || !meter.rebase()) return false;
      if (!activated(model, previous, next, before)) {
        res.error = "the TV accepted the 3D LUT but the picture did not change (firmware ignored it)";
        dev.upload_3d(Slot::Bt709, previous);
        return false;
      }
      lut3d = next;
      if (pass == 0) res.predicted = then;
      // Closed loop: the steering verification set and a random part of the cube, added to the model.
      auto vset = verification_set(0, opt.seed);
      std::vector<Vec3> vxyz;
      if (!measure_set(vset, &vxyz)) return false;
      Stats measured = score(vset, vxyz);
      std::snprintf(b, sizeof b, "colour pass %d measured: %s", pass, measured.str().c_str());
      log(b);
      if (!have_best || measured.colour_avg + measured.grey_avg < best_v.colour_avg + best_v.grey_avg) {
        best_v = measured;
        best = lut3d;
        have_best = true;
      } else {
        break;
      }
      for (size_t i = 0; i < vset.size(); i++) samples.push_back({lut3d.eval(vset[i].signal), vxyz[i], 1});
      std::uniform_int_distribution<size_t> pick(0, set.size() - 1);
      for (int i = 0; i < opt.loop_samples; i++) {
        const Probe &p = set[pick(rng)];
        Vec3 x;
        if (!meter.read(p.patch, &x)) return false;
        samples.push_back({lut3d.eval(p.signal), x, 1});
      }
      if (!model.fit(samples, opt.lattice, &err)) break;
    }
    if (best.node != lut3d.node) {
      if (!dev.upload_3d(Slot::Bt709, best) || !meter.rebase()) return false;
      lut3d = best;
    }
    return true;
  }
};

SdrResult run_sdr(Device &dev, const SdrOptions &options) {
  SdrResult res;
  Run run{dev, Measurer(dev), options, res};
  auto fail = [&](const std::string &why) {
    if (res.error.empty()) res.error = why;
    res.readings = run.meter.readings();
    return res;
  };
  // Light on the TV's own state, then the baseline every result is compared with.
  if (!run.search_light()) return fail("light search failed");
  if (!run.verify(1, &res.baseline)) return fail("meter failed");
  run.log("baseline: " + res.baseline.str());
  // Black, for BT.1886.
  Vec3 black;
  if (!run.meter.read({64, 64, 64}, &black)) return fail("meter failed");
  run.opt.target.Lb = std::max(black.y, 0.0);

  // Bypass, then the pipeline order.
  if (!dev.upload_3d(Slot::Bt709, Lut3D::unity()) || !dev.upload_3d(Slot::Bt2020, Lut3D::unity()) || !dev.upload_1d(Lut1D::unity()))
    return fail("bypass upload failed");
  bool first = options.order == 1;
  if (options.order < 0 && !run.detect_order(&first)) return fail("pipeline order test failed");
  res.lut1d_first = first;
  run.log(first ? "order: 1D LUT before 3D LUT (3D LUT carries everything)" : "order: 3D LUT, then 1D LUT (1D grey, 3D colour)");
  if (!run.meter.rebase()) return fail("meter failed");

  if (!first) {
    if (!run.grey_stage()) return fail("grey stage failed");
  }
  if (!run.colour_stage(!first)) return fail("colour stage failed");

  if (!run.verify(1, &res.final)) return fail("meter failed");
  run.log("final: " + res.final.str());
  res.lut1d = run.lut1d;
  res.lut3d = run.lut3d;
  res.readings = run.meter.readings();
  if (res.final.worse_than(res.baseline)) {
    // Never worse (§9.1): back to the tables the TV started with.
    dev.reset_factory();
    res.error = "the result was worse than the starting point, so it was undone";
    return res;
  }
  res.ok = true;
  return res;
}

}  // namespace cal
