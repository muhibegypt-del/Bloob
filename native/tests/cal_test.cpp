// Calibration tests (CALIBRATION-PLAN.md §10): the colour science against
// published values, LG's table formats against bscpylgtv's, the panel model on
// a non-additive WOLED panel, and the whole SDR procedure on the virtual G2.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "colour.h"
#include "hdr.h"
#include "lut.h"
#include "model.h"
#include "pal.h"
#include "procedure.h"
#include "sim.h"

using namespace cal;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)
#define NEAR(a, b, tol) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) > (tol)) { std::printf("FAIL %s:%d  %s = %.6f, expected %.6f\n", __FILE__, __LINE__, #a, _a, _b); failures++; } } while (0)

// Sharma, Wu and Dalal's CIEDE2000 test data: L a b, L a b, ΔE.
static const double kSharma[34][7] = {
    {50.0000, 2.6772, -79.7751, 50.0000, 0.0000, -82.7485, 2.0425},
    {50.0000, 3.1571, -77.2803, 50.0000, 0.0000, -82.7485, 2.8615},
    {50.0000, 2.8361, -74.0200, 50.0000, 0.0000, -82.7485, 3.4412},
    {50.0000, -1.3802, -84.2814, 50.0000, 0.0000, -82.7485, 1.0000},
    {50.0000, -1.1848, -84.8006, 50.0000, 0.0000, -82.7485, 1.0000},
    {50.0000, -0.9009, -85.5211, 50.0000, 0.0000, -82.7485, 1.0000},
    {50.0000, 0.0000, 0.0000, 50.0000, -1.0000, 2.0000, 2.3669},
    {50.0000, -1.0000, 2.0000, 50.0000, 0.0000, 0.0000, 2.3669},
    {50.0000, 2.4900, -0.0010, 50.0000, -2.4900, 0.0009, 7.1792},
    {50.0000, 2.4900, -0.0010, 50.0000, -2.4900, 0.0010, 7.1792},
    {50.0000, 2.4900, -0.0010, 50.0000, -2.4900, 0.0011, 7.2195},
    {50.0000, 2.4900, -0.0010, 50.0000, -2.4900, 0.0012, 7.2195},
    {50.0000, -0.0010, 2.4900, 50.0000, 0.0009, -2.4900, 4.8045},
    {50.0000, -0.0010, 2.4900, 50.0000, 0.0010, -2.4900, 4.8045},
    {50.0000, -0.0010, 2.4900, 50.0000, 0.0011, -2.4900, 4.7461},
    {50.0000, 2.5000, 0.0000, 50.0000, 0.0000, -2.5000, 4.3065},
    {50.0000, 2.5000, 0.0000, 73.0000, 25.0000, -18.0000, 27.1492},
    {50.0000, 2.5000, 0.0000, 61.0000, -5.0000, 29.0000, 22.8977},
    {50.0000, 2.5000, 0.0000, 56.0000, -27.0000, -3.0000, 31.9030},
    {50.0000, 2.5000, 0.0000, 58.0000, 24.0000, 15.0000, 19.4535},
    {50.0000, 2.5000, 0.0000, 50.0000, 3.1736, 0.5854, 1.0000},
    {50.0000, 2.5000, 0.0000, 50.0000, 3.2972, 0.0000, 1.0000},
    {50.0000, 2.5000, 0.0000, 50.0000, 1.8634, 0.5757, 1.0000},
    {50.0000, 2.5000, 0.0000, 50.0000, 3.2592, 0.3350, 1.0000},
    {60.2574, -34.0099, 36.2677, 60.4626, -34.1751, 39.4387, 1.2644},
    {63.0109, -31.0961, -5.8663, 62.8187, -29.7946, -4.0864, 1.2630},
    {61.2901, 3.7196, -5.3901, 61.4292, 2.2480, -4.9620, 1.8731},
    {35.0831, -44.1164, 3.7933, 35.0232, -40.0716, 1.5901, 1.8645},
    {22.7233, 20.0904, -46.6940, 23.0331, 14.9730, -42.5619, 2.0373},
    {36.4612, 47.8580, 18.3852, 36.2715, 50.5065, 21.2231, 1.4146},
    {90.8027, -2.0831, 1.4410, 91.1528, -1.6435, 0.0447, 1.4441},
    {90.9257, -0.5406, -0.9208, 88.6381, -0.8985, -0.7239, 1.5381},
    {6.7747, -0.2908, -2.4247, 5.8714, -0.0985, -2.2286, 0.6377},
    {2.0776, 0.0795, -1.1350, 0.9033, -0.0636, -0.5514, 0.9082}};

static void test_colour() {
  for (auto &r : kSharma) NEAR(de2000({r[0], r[1], r[2]}, {r[3], r[4], r[5]}), r[6], 1e-4);
  // PQ (ST 2084): known signal levels.
  NEAR(pq_inverse(100), 0.5081, 5e-4);
  NEAR(pq_inverse(1000), 0.7518, 5e-4);
  NEAR(pq_inverse(10000), 1.0, 1e-9);
  NEAR(pq_eotf(pq_inverse(203)), 203, 1e-6);
  // BT.1886 with zero black is a 2.4 power.
  NEAR(bt1886(0.5, 100, 0), 100 * std::pow(0.5, 2.4), 1e-9);
  NEAR(bt1886(1, 100, 0.05), 100, 1e-9);
  NEAR(bt1886(0, 100, 0.05), 0.05, 1e-9);
  // Rec.709 white is D65 with Y = 1; red's luminance is 0.2126.
  Mat3 m = rgb_to_xyz(kRec709);
  Vec3 w = m * Vec3{1, 1, 1};
  NEAR(w.y, 1, 1e-9);
  double x, y;
  chromaticity(w, &x, &y);
  NEAR(x, kD65x, 1e-6);
  NEAR(y, kD65y, 1e-6);
  NEAR((m * Vec3{1, 0, 0}).y, 0.2126, 1e-3);
  // ICtCp: D65 greys are neutral; ΔE ITP is zero for a colour and itself.
  Vec3 g = xyY(kD65x, kD65y, 100), c = ictcp(g);
  NEAR(c.y, 0, 1e-3);
  NEAR(c.z, 0, 1e-3);
  NEAR(de_itp(g, g), 0, 1e-12);
  CHECK(de_itp(g, xyY(0.32, 0.33, 100)) > 1);
}

static void test_luts() {
  // Byte-identical to bscpylgtv's unity tables (unity_lut_1d, unity_lut_3d(33)).
  auto u1 = Lut1D::unity().encode(), u3 = Lut3D::unity().encode();
  CHECK(u1.size() == 3072);
  CHECK(u3.size() == 33u * 33 * 33 * 3);
  CHECK(fnv1a(u1) == 0x44961b908ab9dc05ULL);
  CHECK(fnv1a(u3) == 0x74b28a9148d64cffULL);
  CHECK(u1[1] == 32 && u1[1023] == 32767);
  CHECK(u3[3] == 128 && u3[4] == 0);  // the second node is red + 1 step: red varies fastest
  // Round trips stay within one code.
  Lut3D l = Lut3D::unity();
  for (auto &n : l.node) n = {n.x * 0.9 + 0.05, n.y * n.y, std::sqrt(n.z)};
  Lut3D back = Lut3D::decode(l.encode());
  double worst = 0;
  for (size_t i = 0; i < l.node.size(); i++)
    for (int c = 0; c < 3; c++) worst = std::max(worst, std::fabs(back.node[i][c] - l.node[i][c]));
  CHECK(worst <= 1.0 / 4096 + 1e-12);
  // Tetrahedral interpolation is exact on the unity table.
  Vec3 p = Lut3D::unity().eval({0.123, 0.456, 0.789});
  NEAR(p.x, 0.123, 1e-12);
  NEAR(p.y, 0.456, 1e-12);
  NEAR(p.z, 0.789, 1e-12);
  CHECK(base64({0x4241, 0x43}) == "QUJDAA==");
}

static void test_model() {
  // On the virtual G2 (white sub-pixel, so colours don't add up), the fitted
  // model predicts colours it was not fitted on to well within 1 ΔE.
  VirtualTv tv(panel_g2(), SimMode::Sdr, 3);
  tv.set_light(30);
  auto drive = [&](Vec3 s) {
    Patch p{code_limited(s.x), code_limited(s.y), code_limited(s.z)};
    Vec3 x;
    tv.measure(p, &x);
    return Sample{{signal_limited(p.r), signal_limited(p.g), signal_limited(p.b)}, x, 1};
  };
  std::vector<Sample> s;
  for (int c = 0; c < 3; c++)
    for (int i = 1; i <= 32; i++) {
      Vec3 v;
      v[c] = i / 32.0;
      s.push_back(drive(v));
    }
  s.push_back(drive({0, 0, 0}));
  for (int b = 0; b < 9; b++)
    for (int g = 0; g < 9; g++)
      for (int r = 0; r < 9; r++) s.push_back(drive({r / 8.0, g / 8.0, b / 8.0}));
  PanelModel m;
  std::string err;
  CHECK(m.fit(s, 11, &err));
  Vec3 white = m.predict({1, 1, 1});
  double sum = 0, worst = 0;
  int n = 0;
  for (double r = 0.07; r < 1; r += 0.17)
    for (double g = 0.11; g < 1; g += 0.19)
      for (double b = 0.03; b < 1; b += 0.23) {
        Patch p{code_limited(r), code_limited(g), code_limited(b)};
        double de = de2000_xyz(m.predict({signal_limited(p.r), signal_limited(p.g), signal_limited(p.b)}), tv.render(p), white);
        sum += de;
        worst = std::max(worst, de);
        n++;
      }
  std::printf("  model on held-out colours: %.3f avg, %.3f max dE2000 (%d), lambda %g\n", sum / n, worst, n, m.lambda());
  CHECK(sum / n < 0.6);
  CHECK(worst < 2.0);
  // Inversion finds the drive for a reachable colour.
  Vec3 target = m.predict({0.6, 0.3, 0.2}), x{0.5, 0.5, 0.5};
  CHECK(m.invert(target, white, &x) < 0.05);
}

static void report(const char *name, const SdrResult &r) {
  std::printf("  %s: %s, light %d, %ld readings, model %.2f\n", name, r.ok ? "ok" : r.error.c_str(), r.light, r.readings, r.model_holdout);
  std::printf("    before: %s\n    after:  %s\n", r.baseline.str().c_str(), r.final.str().c_str());
  if (std::getenv("CAL_VERBOSE"))
    for (const auto &l : r.log) std::printf("    | %s\n", l.c_str());
}

static void meets_pass_marks(const SdrResult &r) {
  CHECK(r.ok);
  CHECK(r.final.grey_avg <= 0.5);
  CHECK(r.final.grey_max <= 1.0);
  CHECK(r.final.colour_avg <= 1.0);
  CHECK(r.final.colour_max <= 2.0);
}

static void test_sdr() {
  SdrOptions o;
  {
    VirtualTv tv(panel_g2(), SimMode::Sdr, 11);
    SdrResult r = run_sdr(tv, o);
    report("G2", r);
    CHECK(tv.bad_uploads() == 0);
    meets_pass_marks(r);
    CHECK(!r.lut1d_first);
    CHECK(r.baseline.grey_avg > 2);  // the uncalibrated panel really was off
  }
  {
    VirtualTv tv(panel_hard(), SimMode::Sdr, 12);
    SdrResult r = run_sdr(tv, o);
    report("hard panel", r);
    CHECK(tv.bad_uploads() == 0);
    meets_pass_marks(r);
  }
  {
    VirtualTv tv(panel_drifting(), SimMode::Sdr, 13);
    SdrResult r = run_sdr(tv, o);
    report("dimming panel", r);
    meets_pass_marks(r);
  }
  {
    PanelSpec s = panel_g2();
    s.lut1d_first = true;
    VirtualTv tv(s, SimMode::Sdr, 14);
    SdrResult r = run_sdr(tv, o);
    report("1D-first pipeline", r);
    CHECK(tv.bad_uploads() == 0);
    CHECK(r.lut1d_first);
    meets_pass_marks(r);
  }
  {
    PanelSpec s = panel_g2();
    s.ignores_3d = true;
    VirtualTv tv(s, SimMode::Sdr, 15);
    SdrResult r = run_sdr(tv, o);
    report("firmware ignores 3D LUT", r);
    CHECK(!r.ok);
    CHECK(r.error.find("firmware") != std::string::npos);
  }
  {
    // A brighter target with a power 2.2 curve.
    SdrOptions b = o;
    b.target.Lw = 140;
    b.target.bt1886 = false;
    b.target.gamma = 2.2;
    VirtualTv tv(panel_g2(), SimMode::Sdr, 16);
    SdrResult r = run_sdr(tv, b);
    report("140 nits, gamma 2.2", r);
    meets_pass_marks(r);
  }
}

// The profile the TV runs (fewer readings, since a Spyder5 takes seconds per reading),
// at the owner's 140 nits: still within the pass marks, in a bounded number of readings.
static void test_sdr_tv_profile() {
  for (int k = 0; k < 2; k++) {
    VirtualTv tv(k ? panel_hard() : panel_g2(), SimMode::Sdr, 51 + k);
    SdrOptions o = SdrOptions::tv_profile();
    o.target.Lw = 140;
    SdrResult r = run_sdr(tv, o);
    report(k ? "TV profile, hard panel, 140 nits" : "TV profile, G2, 140 nits", r);
    meets_pass_marks(r);
    CHECK(r.readings < 6000);  // about 3,900 on the G2; the hard panel needs more grey passes
    CHECK(tv.bad_uploads() == 0);
  }
}

// Quick (close to Calman's AutoCal): a third of the readings. The G2 must still meet the
// pass marks; the deliberately hard panel and a harsh meter get the harsh-meter marks.
static void test_sdr_quick() {
  for (int k = 0; k < 3; k++) {
    PanelSpec s = k == 1 ? panel_hard() : panel_g2();
    if (k == 2) {
      s.noise = 0.0005;
      s.chroma_noise = 0.002;
    }
    VirtualTv tv(s, SimMode::Sdr, 61 + k);
    SdrOptions o = SdrOptions::quick();
    o.target.Lw = 140;
    SdrResult r = run_sdr(tv, o);
    report(k == 0 ? "quick, G2, 140 nits" : k == 1 ? "quick, hard panel" : "quick, harsh meter", r);
    CHECK(tv.bad_uploads() == 0);
    if (k == 0) {
      meets_pass_marks(r);
      CHECK(r.readings < 2200);  // about 1,900: half the thorough run
    } else {
      CHECK(r.ok);
      CHECK(r.final.grey_avg <= 0.7);
      CHECK(r.final.grey_max <= 2.0);
      CHECK(r.final.colour_avg <= 1.0);
      CHECK(r.final.colour_max <= 2.0);
      CHECK(r.readings < 3200);
    }
  }
}

// A harsher meter than any colorimeter should be (independent 0.2% noise on X, Y and Z):
// the result must degrade gracefully, not fall apart.
static void test_sdr_harsh_meter() {
  PanelSpec s = panel_g2();
  s.noise = 0.0005;
  s.chroma_noise = 0.002;
  VirtualTv tv(s, SimMode::Sdr, 21);
  SdrResult r = run_sdr(tv, SdrOptions{});
  report("harsh meter", r);
  CHECK(r.ok);
  CHECK(r.final.grey_avg <= 0.7);
  CHECK(r.final.colour_avg <= 1.0);
  CHECK(r.final.grey_max <= 2.0);
}

static void test_hdr10() {
  VirtualTv tv(panel_g2(), SimMode::Hdr10, 31);
  HdrResult r = run_hdr10(tv, HdrOptions{});
  std::printf("  HDR10: %s, peak %.0f, matrix %s, %ld readings\n", r.ok ? "ok" : r.error.c_str(), r.peak, r.matrix_kept ? "kept" : "not kept", r.readings);
  std::printf("    before:  %s\n    without: %s\n    with:    %s\n    after:   %s\n", r.baseline.str().c_str(), r.without_matrix.str().c_str(),
              r.with_matrix.str().c_str(), r.final.str().c_str());
  if (std::getenv("CAL_VERBOSE"))
    for (const auto &l : r.log) std::printf("    | %s\n", l.c_str());
  CHECK(r.ok);
  CHECK(tv.bad_uploads() == 0);
  NEAR(r.peak, 780, 20);
  CHECK(r.final.grey_avg <= 2.0);
  CHECK(r.final.colour_avg <= 3.0);
  CHECK(r.matrix_kept);
  CHECK(r.highlights_monotonic);
  CHECK(tv.tonemap()[0] == r.tonemap[0] && r.tonemap[1] == 1000);
}

static void test_dolby() {
  {
    VirtualTv tv(panel_g2(), SimMode::DolbyVision, 41);
    DolbyResult r = run_dolby(tv, DolbyOptions{});
    std::printf("  Dolby Vision: %s, Tmax %.1f, peak after %.1f, config %s, %ld readings\n", r.ok ? "ok" : r.error.c_str(), r.tmax, r.peak_after,
                r.config_loaded ? "loaded" : "not loaded", r.readings);
    std::printf("    before: %s\n    after:  %s\n", r.baseline.str().c_str(), r.final.str().c_str());
    if (std::getenv("CAL_VERBOSE"))
      for (const auto &l : r.log) std::printf("    | %s\n", l.c_str());
    CHECK(r.ok);
    CHECK(tv.bad_uploads() == 0);
    CHECK(r.passthrough);
    CHECK(r.config_loaded);  // loaded before any 1D upload, so the pop-up was still offered
    CHECK(r.peak_matches);
    CHECK(r.black_ok);
    CHECK(r.final.grey_avg <= 1.0);
    CHECK(tv.dolby_config().find("[PictureMode = 2]\r\n") != std::string::npos);
    CHECK(tv.dolby_config().find("Tgamma = 2.2\r\n") != std::string::npos);
  }
  {
    PanelSpec s = panel_g2();
    s.usb_stick = false;
    VirtualTv tv(s, SimMode::DolbyVision, 42);
    DolbyResult r = run_dolby(tv, DolbyOptions{});
    CHECK(r.ok);
    CHECK(!r.config_loaded);
  }
  {
    PanelSpec s = panel_g2();
    s.dolby_mapped = true;
    VirtualTv tv(s, SimMode::DolbyVision, 43);
    DolbyResult r = run_dolby(tv, DolbyOptions{});
    std::printf("  Dolby Vision without pass-through: %s\n", r.error.c_str());
    CHECK(!r.ok);
    CHECK(!r.passthrough);
  }
  // The configuration file in bscpylgtv's 2019 layout.
  const double p[6] = {0.68, 0.32, 0.265, 0.69, 0.15, 0.06};
  std::string c = dolby_config(2, 750.5, 0.0001, 2.2, p);
  CHECK(c.find("Tmax = 750.5000\r\n") != std::string::npos);
  CHECK(c.find("TPrimaries = 0.6800 0.3200 0.2650 0.6900 0.1500 0.0600 0.3127 0.3290\r\n") != std::string::npos);
}

// ---- Pal's grey flow (research/Pal-in-Bloob.md §9–11) ----------------------

// One grey report on a Device, as bloob-cal's grey-report takes it: two readings a level
// (averaged), 0% not read, and the three primaries at 100%.
static std::vector<GreyLevel> pal_report(Device &tv, Vec3 prim[3]) {
  std::vector<GreyLevel> levels;
  for (int pct : pal_levels()) {
    GreyLevel l;
    l.pct = pct;
    l.code = pal_code(pct);
    if (pct > 0) {
      Vec3 a, b;
      tv.measure({l.code, l.code, l.code}, &a);
      tv.measure({l.code, l.code, l.code}, &b);
      l.XYZ = (a + b) / 2.0;
      l.measured = true;
      l.reads = 2;
    }
    levels.push_back(l);
  }
  const Patch ps[3] = {{940, 64, 64}, {64, 940, 64}, {64, 64, 940}};
  for (int c = 0; c < 3; c++) tv.measure(ps[c], &prim[c]);
  return levels;
}

struct PalRun {
  GreySummary before, after;
  int passes = 0, light = 0;
  bool goal = false, worse = false;
  std::vector<double> avgs;
};

// The scripted Pal: light with headroom, then report → propose → gate → upload → report,
// keeping each pass only if it is better; at most three passes.
static PalRun pal_flow(VirtualTv &tv, const SdrTarget &t, TableIndex mode = TableIndex::Signal) {
  PalRun run;
  PalGoal goal;
  Lut1D table = Lut1D::unity();
  tv.upload_1d(table);
  Vec3 w;
  for (int i = 0; i < 6; i++) {  // white about 6% above the target, as pal-start sets it
    tv.measure({940, 940, 940}, &w);
    int light = static_cast<int>(std::lround(tv.light() * t.Lw * 1.06 / std::max(w.y, 1.0)));
    if (light == tv.light()) break;
    tv.set_light(std::clamp(light, 0, 100));
  }
  run.light = tv.light();
  Vec3 prim[3];
  auto levels = pal_report(tv, prim);
  run.before = pal_summarise(levels, t, goal);
  GreySummary now = run.before;
  run.avgs.push_back(now.avg);
  for (int pass = 1; pass <= 3 && !now.goal_met; pass++) {
    // White short of the target (the correction trims channels): more light, then a fresh report.
    if (now.white_err_pct < -PalGoal{}.white_pct / 2) {
      tv.set_light(std::clamp(static_cast<int>(std::ceil(tv.light() * t.Lw / std::max(now.white_Y, 1.0) * 1.01)), 0, 100));
      levels = pal_report(tv, prim);
      now = pal_summarise(levels, t, goal);
      if (now.goal_met) break;
    }
    std::vector<std::string> notes;
    PalAdjust a = pal_propose(levels, table, prim, t, pass == 1 ? 1.0 : 0.8, mode, &notes);
    std::string why;
    CHECK(pal_adjust_valid(a, &why));
    Lut1D next = pal_apply(table, a, mode);
    GateResult g = pal_gate(next, table);
    if (!g.ok) std::printf("    gate: %s\n", g.why.c_str());
    CHECK(g.ok);
    if (!g.ok) break;
    tv.upload_1d(next);
    Vec3 p2[3];
    auto after = pal_report(tv, p2);
    GreySummary s = pal_summarise(after, t, goal);
    run.passes = pass;
    run.avgs.push_back(s.avg);
    for (size_t k = 0; k < after.size(); k++)
      if (after[k].scored && after[k].de > run.before.max + 0.3 && after[k].de > levels[k].de + 0.3) run.worse = true;
    table = next;
    levels = after;
    for (int c = 0; c < 3; c++) prim[c] = p2[c];
    now = s;
  }
  if (std::getenv("CAL_VERBOSE"))
    for (auto &l : levels)
      if (l.scored) std::printf("      %3d%%  de %.2f  Y %.3f / %.3f\n", l.pct, l.de, l.XYZ.y, l.target.y);
  run.after = now;
  run.goal = now.goal_met;
  run.light = tv.light();
  return run;
}

static void test_pal() {
  // Levels and targets.
  auto lv = pal_levels();
  CHECK(lv.size() == 21 && lv.front() == 0 && lv.back() == 100);
  CHECK(pal_code(0) == 64 && pal_code(100) == 940 && pal_code(50) == 502);
  NEAR(pal_table_x(100, TableIndex::Signal), 1.0, 1e-12);
  NEAR(pal_table_x(100, TableIndex::Code), 940 / 1023.0, 1e-12);
  SdrTarget t;
  t.Lw = 140;
  NEAR(pal_target(t, 100).y, 140, 1e-9);
  NEAR(pal_target(t, 50).y, 140 * std::pow(0.5, 2.4), 1e-6);

  // A summary that meets the goal, and one that misses it at one level.
  std::vector<GreyLevel> g;
  for (int pct : lv) {
    GreyLevel l;
    l.pct = pct;
    l.measured = pct > 0;
    l.XYZ = pal_target(t, pct);
    g.push_back(l);
  }
  GreySummary s = pal_summarise(g, t, PalGoal{});
  CHECK(s.goal_met);
  NEAR(s.avg, 0, 1e-9);
  g[10].XYZ = g[10].XYZ * 1.05;  // 50% 5% bright
  s = pal_summarise(g, t, PalGoal{});
  CHECK(!s.goal_met || s.max > 0.3);
  CHECK(s.max_at == 50);
  CHECK(!g[1].scored && g[2].scored);  // 5% reported, not scored; 10% scored

  // Sanity: a pink flicker, a darker step, and a dark patch whose colour can't be judged.
  std::string why;
  Vec3 grey = xyY(kD65x, kD65y, 30), pink = grey + Vec3{8, 3, 12};
  CHECK(pal_grey_sane(grey, nullptr, 0.3, &why));
  CHECK(!pal_grey_sane(pink, nullptr, 0.3, &why) && why == "far from neutral");
  Vec3 brighter = xyY(kD65x, kD65y, 40);
  CHECK(!pal_grey_sane(grey, &brighter, 0.3, &why) && why == "darker than the level below");
  CHECK(pal_grey_sane(xyY(0.25, 0.25, 0.2), nullptr, 0.3, &why));

  // Adjustments: validation, identity, and the table's own shape kept between levels.
  PalAdjust a;
  a.pct = lv;
  for (auto &m : a.mult) m.assign(lv.size(), 1.0);
  CHECK(pal_adjust_valid(a, &why));
  Lut1D base = Lut1D::unity();
  for (int i = 0; i < Lut1D::kSize; i++) base.ch[0][i] = std::pow(i / 1023.0, 1.05);
  Lut1D same = pal_apply(base, a);
  for (int c = 0; c < 3; c++)
    for (int i = 0; i < Lut1D::kSize; i++) NEAR(same.ch[c][i], base.ch[c][i], 1e-12);
  a.mult[2][10] = 0.97;  // blue at 50%
  Lut1D bent = pal_apply(base, a);
  NEAR(bent.eval(2, 0.5), 0.5 * 0.97, 1e-4);
  NEAR(bent.eval(2, 0.25), 0.25, 1e-9);  // the next untouched levels stay put
  CHECK(pal_gate(bent, base).ok);
  PalAdjust bad = a;
  bad.mult[1].pop_back();
  CHECK(!pal_adjust_valid(bad, &why));
  bad = a;
  bad.mult[0][3] = 2.0;
  CHECK(!pal_adjust_valid(bad, &why));
  bad = a;
  bad.pct[4] = bad.pct[3];
  CHECK(!pal_adjust_valid(bad, &why));

  // The gate: a falling table, a value out of range, a big step, a spike, and a NaN.
  Lut1D u = Lut1D::unity(), x = u;
  x.ch[1][500] = x.ch[1][499] - 0.01;
  CHECK(!pal_gate(x, u).ok);
  x = u;
  x.ch[0][1023] = 1.2;
  CHECK(!pal_gate(x, u).ok);
  x = u;
  for (int i = 0; i < 1024; i++) x.ch[2][i] *= 0.8;
  GateResult gr = pal_gate(x, u);
  CHECK(!gr.ok && gr.why.find("allowed in one pass") != std::string::npos);
  x = u;
  x.ch[0][300] += 0.004;
  x.ch[0][301] += 0.004;
  gr = pal_gate(x, u);
  CHECK(!gr.ok);
  x = u;
  x.ch[0][10] = std::nan("");
  CHECK(!pal_gate(x, u).ok);
  CHECK(pal_gate(u, u).ok);

  // The index check's flat step stays a rising table and darkens only its band.
  Lut1D p = pal_plateau(u, 0.77, 0.83);
  CHECK(pal_gate(p, u, 0.15, false).ok);
  CHECK(!pal_gate(p, u).ok);  // a deliberate kink: only the index check may write one
  NEAR(p.eval(0, 0.8), 0.77, 2e-3);
  NEAR(p.eval(0, 0.748), 0.748, 1e-9);

  // The whole flow on the virtual G2 and the hard panel, at the owner's 140 nits.
  struct Case {
    const char *name;
    PanelSpec spec;
    unsigned seed;
  };
  std::vector<Case> cases;
  for (unsigned seed = 61; seed < 69; seed++) cases.push_back({"G2", panel_g2(), seed});
  for (unsigned seed = 71; seed < 75; seed++) cases.push_back({"hard panel", panel_hard(), seed});
  for (unsigned seed = 81; seed < 84; seed++) cases.push_back({"dimming panel", panel_drifting(), seed});
  int met = 0;
  for (const Case &c : cases) {
    VirtualTv tv(c.spec, SimMode::Sdr, c.seed);
    PalRun r = pal_flow(tv, t);
    std::printf("  Pal grey, %s (seed %u): light %d, before %.2f avg / %.2f max, after %.2f / %.2f in %d pass%s%s\n", c.name, c.seed, r.light,
                r.before.avg, r.before.max, r.after.avg, r.after.max, r.passes, r.passes == 1 ? "" : "es", r.goal ? ", goal met" : ", GOAL MISSED");
    CHECK(tv.bad_uploads() == 0);
    CHECK(!r.worse);
    CHECK(r.after.avg < r.before.avg);
    if (std::string(c.name) == "G2") CHECK(r.goal && r.passes <= 2);
    met += r.goal;
  }
  std::printf("  Pal grey: goal met in %d of %zu runs\n", met, cases.size());
  CHECK(met >= static_cast<int>(cases.size()) - 1);
}

int main() {
  test_colour();
  test_luts();
  test_model();
  test_sdr();
  test_sdr_harsh_meter();
  test_sdr_tv_profile();
  test_sdr_quick();
  test_hdr10();
  test_dolby();
  test_pal();
  if (failures) std::printf("%d calibration check(s) failed\n", failures);
  else std::printf("calibration tests passed\n");
  return failures ? 1 : 0;
}
