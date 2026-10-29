// bloob-cal: calibration on the TV, run as root over SSH (CALIBRATION-PLAN.md §11).
//   bloob-cal check   read-only: tables can be read, the meter session, how steady it reads,
//                     whether SDR patterns are limited range. Changes nothing.
//   bloob-cal test    the Expert 2 test (C2): saves Expert 2's tables, writes test tables,
//                     checks the picture changed as expected, puts Expert 2 back to factory,
//                     and returns to the picture mode you were in. Expert 1 is never touched.
// Every run ends calibration mode and turns the patterns off, also on Ctrl-C.
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include "colour.h"
#include "lut.h"
#include "tv_device.h"

using namespace cal;

static const char *kProbe = "/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service";
static const char *kData = "/media/internal/com.muhib.bloob/calibration";

static std::string env_or(const char *name, const std::string &fallback) {
  const char *v = getenv(name);
  return v && *v ? v : fallback;
}

static void on_signal(int) { g_stop = 1; }

static bool exists(const std::string &p) { return access(p.c_str(), F_OK) == 0; }

static TvConfig config(const std::string &mode) {
  std::string probe = env_or("BLOOB_PROBE", kProbe), data = env_or("BLOOB_CAL_DATA", kData);
  TvConfig c;
  c.meter.spotread = probe + "/spotread";
  c.meter.libs = probe + "/libs";
  c.meter.loader = env_or("BLOOB_LOADER", "/lib/ld-linux.so.3");
  if (c.meter.loader == "none") c.meter.loader.clear();
  std::string ccss = data + "/argyll/lg-55g2.ccss";
  if (exists(ccss)) c.meter.ccss = ccss;
  c.manifest = probe + "/itpg_manifest.json";
  c.key_file = data + "/ssap-key.json";
  c.picture_mode = mode;
  c.verbose = getenv("BLOOB_VERBOSE") != nullptr;
  return c;
}

struct Reading {
  Vec3 xyz;
  double x = 0, y = 0;
};

static bool read_avg(TvDevice &tv, const Patch &p, int n, Reading *out, double *spread = nullptr) {
  std::vector<Vec3> v;
  for (int i = 0; i < n && !g_stop; i++) {
    Vec3 x;
    if (!tv.measure(p, &x)) {
      std::printf("    reading failed: %s\n", tv.last_error().c_str());
      return false;
    }
    v.push_back(x);
  }
  if (v.empty()) return false;
  Vec3 m;
  for (auto &x : v) m = m + x / static_cast<double>(v.size());
  out->xyz = m;
  chromaticity(m, &out->x, &out->y);
  if (spread) {
    double s = 0;
    for (auto &x : v) s += (x.y - m.y) * (x.y - m.y);
    *spread = v.size() > 1 && m.y > 0 ? 100 * std::sqrt(s / (v.size() - 1)) / m.y : 0;
  }
  return true;
}

static void show(const char *label, const Reading &r) {
  std::printf("  %-34s Y %8.3f cd/m2   x %.4f  y %.4f\n", label, r.xyz.y, r.x, r.y);
}

static int pass_count = 0, fail_count = 0;
static void verdict(bool ok, const std::string &what) {
  std::printf("  %s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  (ok ? pass_count : fail_count)++;
}

static void tables(TvDevice &tv) {
  std::printf("== Tables (read-only)\n");
  const char *cmds[] = {"GET_1D_DPG_DATA", "GET_3D_LUT_DATA", "GET_3BY3_GAMUT_DATA"};
  const char *names[] = {"1D LUT", "3D LUT", "3x3 matrix"};
  for (int i = 0; i < 3; i++) {
    Json r;
    if (!tv.get_table(cmds[i], &r)) {
      std::printf("  %-10s no reply (%s)\n", names[i], tv.last_error().c_str());
      continue;
    }
    std::printf("  %-10s %s, %lld values, %s%s\n", names[i], r["returnValue"].as_bool() ? "readable" : "not readable", r["dataCount"].as_int(),
                r["dataType"].as_string("?").c_str(), r.has("errorText") ? (" (" + r["errorText"].as_string() + ")").c_str() : "");
  }
}

static bool meter_up(TvDevice &tv) {
  std::printf("== Meter\n");
  std::string err;
  if (!tv.open_meter(&err)) {
    std::printf("  The meter did not answer: %s\n  Is the Spyder5 plugged into the TV?\n", err.c_str());
    return false;
  }
  std::printf("  %s\n", tv.meter_session() ? "one session open (fast readings)" : "one reading per start (slower; the session did not open)");
  std::printf("  correction: %s\n", config("expert1").meter.ccss.empty() ? "none found (readings uncorrected)" : "LG 55G2 (i1 Pro 2)");
  return true;
}

static int cmd_check() {
  TvDevice tv(config("expert1"));
  Json pic;
  if (tv.read_picture(&pic)) std::printf("== Picture now: %s\n", Json::stringify(pic).c_str());
  tables(tv);
  if (!meter_up(tv)) return 1;
  std::printf("== Readings (keep the meter still and the room dark)\n");
  Reading w, hi;
  double spread = 0;
  auto t0 = std::chrono::steady_clock::now();
  if (!read_avg(tv, {940, 940, 940}, 10, &w, &spread)) return 1;
  double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 10;
  show("white 940 (10 readings)", w);
  std::printf("  steadiness: %.2f%% spread, %.1f s per reading\n", spread, secs);
  if (!read_avg(tv, {1019, 1019, 1019}, 3, &hi)) return 1;
  show("white 1019 (range test)", hi);
  double ratio = hi.xyz.y / std::max(w.xyz.y, 1e-6);
  std::printf("  range: 1019 is %.2fx 940 -> %s\n", ratio, ratio > 1.12 ? "full range" : ratio > 1.02 ? "limited, above-white passed" : "limited range");
  tv.finish();
  std::printf("Done. Nothing was changed.\n");
  return 0;
}

static int cmd_test() {
  TvDevice probe(config("expert1"));
  Json pic;
  if (!probe.read_picture(&pic)) {
    std::printf("Could not read the picture settings: %s\n", probe.last_error().c_str());
    return 1;
  }
  std::string home_mode = pic["pictureMode"].as_string("expert1");
  if (home_mode == "expert2") home_mode = "expert1";
  std::printf("== You are in %s; the test uses expert2 and comes back to %s.\n", pic["pictureMode"].as_string("?").c_str(), home_mode.c_str());

  TvDevice tv(config("expert2"));
  tables(tv);
  if (!meter_up(tv)) return 1;
  if (!tv.set_picture_mode("expert2")) {
    std::printf("Could not switch to expert2: %s\n", tv.last_error().c_str());
    return 1;
  }
  sleep(2);
  bool wrote = false;
  auto restore = [&]() {
    if (wrote && g_stop) {
      // Stopped part-way: Expert 2 still goes back to factory before anything else.
      int stop = g_stop;
      g_stop = 0;
      bool ok = tv.set_calibration_mode(true) && tv.reset_factory();
      std::printf("  %s\n", ok ? "Stopped: Expert 2 put back to factory." : ("Stopped, and Expert 2 could not be reset: " + tv.last_error()).c_str());
      g_stop = stop;
    }
    tv.finish();
    if (!tv.set_picture_mode(home_mode)) std::printf("  Could not switch back to %s: %s\n", home_mode.c_str(), tv.last_error().c_str());
    else std::printf("  Back in %s.\n", home_mode.c_str());
  };

  // Save Expert 2's own tables before anything is written.
  std::string dir = env_or("BLOOB_CAL_DATA", kData) + "/backups";
  mkdir(env_or("BLOOB_CAL_DATA", kData).c_str(), 0755);
  mkdir(dir.c_str(), 0755);
  char stamp[32];
  time_t now = time(nullptr);
  strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", localtime(&now));
  Json before1d;
  bool have_backup = tv.get_table("GET_1D_DPG_DATA", &before1d) && before1d["returnValue"].as_bool();
  if (have_backup) {
    std::ofstream(dir + "/expert2-" + stamp + "-1d.json") << Json::stringify(before1d);
    std::printf("== Saved Expert 2's 1D LUT to %s/expert2-%s-1d.json\n", dir.c_str(), stamp);
  }

  std::printf("== Before (Expert 2 as it is)\n");
  Reading w0, r0, g0;
  if (!read_avg(tv, {940, 940, 940}, 2, &w0) || !read_avg(tv, {721, 64, 64}, 2, &r0) || !read_avg(tv, {502, 502, 502}, 2, &g0)) {
    restore();
    return 1;
  }
  show("white", w0);
  show("red 75%", r0);
  show("grey 50%", g0);

  std::printf("== Writing test tables (Expert 2 only)\n");
  if (!tv.set_calibration_mode(true)) {
    std::printf("  Calibration mode refused: %s\n", tv.last_error().c_str());
    restore();
    return 1;
  }
  verdict(true, "calibration mode started for expert2");
  wrote = true;
  bool ok = tv.upload_1d(Lut1D::unity());
  verdict(ok, "a neutral 1D LUT was accepted" + (ok ? std::string() : ": " + tv.last_error()));
  Reading w1, r1;
  if (ok && read_avg(tv, {940, 940, 940}, 2, &w1) && read_avg(tv, {721, 64, 64}, 2, &r1)) {
    show("white, neutral 1D LUT", w1);
    show("red, neutral 1D LUT", r1);
  }
  Lut1D half = Lut1D::unity();
  for (double &v : half.ch[0]) v *= 0.5;
  ok = ok && tv.upload_1d(half);
  Reading r2;
  if (ok && read_avg(tv, {721, 64, 64}, 2, &r2)) {
    show("red, red halved", r2);
    verdict(r2.xyz.y < 0.6 * r1.xyz.y, "halving red in the table darkened red on screen (the TV applies 1D tables)");
  } else {
    verdict(false, "halved-red table: " + tv.last_error());
  }

  // Which table acts last (C2.3): a 3D LUT that swaps red and green, and a 1D LUT that blanks red.
  std::printf("== Pipeline order (needs the 3D LUT, sent over the TV's socket; the TV may ask to allow Bloob)\n");
  Lut3D swap = Lut3D::unity();
  for (auto &n : swap.node) std::swap(n.x, n.y);
  Lut1D blank = Lut1D::unity();
  for (double &v : blank.ch[0]) v = 0;
  if (tv.upload_3d(Slot::Bt709, swap) && tv.upload_1d(blank)) {
    Reading o;
    if (read_avg(tv, {721, 64, 64}, 2, &o)) {
      show("red input, swap + blank", o);
      bool green = o.xyz.y > 0.5 && o.y > 0.5;
      std::printf("  order: %s\n", o.xyz.y < 0.5 ? "1D LUT first, then 3D LUT" : green ? "3D LUT first, then 1D LUT (the expected order)" : "unclear");
      verdict(o.xyz.y < 0.5 || green, "the TV applies 3D tables, and the order is known");
    }
    tv.upload_3d(Slot::Bt709, Lut3D::unity());
  } else {
    verdict(false, "3D LUT: " + tv.last_error());
  }

  std::printf("== Back to factory (Expert 2)\n");
  ok = tv.reset_factory();
  verdict(ok, "factory reset accepted" + (ok ? std::string() : ": " + tv.last_error()));
  Reading w3, r3;
  if (ok && read_avg(tv, {940, 940, 940}, 2, &w3) && read_avg(tv, {721, 64, 64}, 2, &r3)) {
    show("white after reset", w3);
    show("red after reset", r3);
    double dw = std::fabs(w3.xyz.y / w0.xyz.y - 1) * 100;
    double dxy = std::hypot(w3.x - w0.x, w3.y - w0.y);
    verdict(dw < 2 && dxy < 0.003, "Expert 2 is back to how it was (white " + std::to_string(dw).substr(0, 4) + "% and " +
                                       std::to_string(dxy).substr(0, 6) + " xy away)");
  }
  if (have_backup) {
    Json after;
    if (tv.get_table("GET_1D_DPG_DATA", &after))
      verdict(after["data"].as_string() == before1d["data"].as_string(), "the 1D LUT read back after the reset equals the saved one");
  }
  tv.set_calibration_mode(false);
  restore();
  std::printf("== %d passed, %d failed. Expert 1 was not touched.\n", pass_count, fail_count);
  return fail_count ? 2 : 0;
}

int main(int argc, char **argv) {
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
  signal(SIGHUP, on_signal);
  signal(SIGPIPE, SIG_IGN);
  setvbuf(stdout, nullptr, _IOLBF, 0);
  std::string cmd = argc > 1 ? argv[1] : "";
  if (geteuid() != 0 && !getenv("BLOOB_CAL_ANY_USER")) {
    std::printf("Run bloob-cal on the TV as root.\n");
    return 1;
  }
  if (cmd == "check") return cmd_check();
  if (cmd == "test") return cmd_test();
  std::printf("usage: bloob-cal check | test\n");
  return 1;
}
