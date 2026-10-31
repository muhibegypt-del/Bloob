// bloob-cal: calibration on the TV (CALIBRATION-PLAN.md §11), run as root. Bloob's
// Settings → Picture → Calibrate starts it through the Homebrew Channel and reads
// its status file; it also runs over SSH.
//   bloob-cal check         read-only: tables can be read, the meter, how steady it reads,
//                           whether SDR patterns are limited range. Changes nothing.
//   bloob-cal test          the Expert 2 test (C2): saves Expert 2's tables, writes test tables,
//                           checks the picture changed, puts Expert 2 back to factory and returns
//                           to the picture mode you were in. Expert 1 is never touched.
//   bloob-cal sdr [nits]    calibrates Expert 1 (default 140 cd/m²): saves your settings and
//                           tables first; a failed or stopped run is undone automatically.
//   bloob-cal undo          Expert 1 back to factory, your saved settings put back.
// Status: <data>/status.json (state, step, readings, recent lines, result), for the screen.
// One run at a time (<data>/bloob-cal.pid). Every run ends calibration mode and turns the
// patterns off, also when stopped (Ctrl-C, SIGTERM from Bloob's Stop, or a dropped SSH session).
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <thread>
#include <string>
#include <vector>

#include "colour.h"
#include "lut.h"
#include "pal.h"
#include "procedure.h"
#include "tv_device.h"

using namespace cal;

static const char *kProbe = "/media/developer/apps/usr/palm/services/org.codex.spyderprobe.service";
static const char *kData = "/media/internal/com.muhib.bloob/calibration";

static std::string env_or(const char *name, const std::string &fallback) {
  const char *v = getenv(name);
  return v && *v ? v : fallback;
}

static void on_signal(int) { g_stop = 1; }

static std::string data_dir() { return env_or("BLOOB_CAL_DATA", kData); }

// The status file Bloob's Calibrate screen reads (written atomically, readable by the app).
struct Status {
  std::string path, action, state = "running", step;
  long readings = 0, expected = 0;
  std::string note;  // what is happening right now, e.g. a slow dark reading
  time_t started = time(nullptr);
  std::vector<std::string> lines;
  std::string result = "null";
  void write() const {
    if (path.empty()) return;
    std::string j = "{\"action\":" + Json::quote(action) + ",\"state\":" + Json::quote(state) + ",\"step\":" + Json::quote(step) +
                    ",\"note\":" + Json::quote(note) + ",\"readings\":" + std::to_string(readings) + ",\"expected\":" + std::to_string(expected) +
                    ",\"started\":" + std::to_string(static_cast<long long>(started)) + ",\"updated\":" +
                    std::to_string(static_cast<long long>(time(nullptr))) + ",\"pid\":" + std::to_string(getpid()) + ",\"lines\":[";
    for (size_t i = 0; i < lines.size(); i++) j += (i ? "," : "") + Json::quote(lines[i]);
    j += "],\"result\":" + result + "}";
    std::string tmp = path + ".tmp";
    int f = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f < 0) return;
    bool ok = ::write(f, j.data(), j.size()) == static_cast<ssize_t>(j.size());
    ::close(f);
    if (ok) ::rename(tmp.c_str(), path.c_str());
  }
};
static Status g_status;

// Everything bloob-cal says goes to its output and to the status file's recent lines;
// "== " lines name the current step.
static FILE *g_log = nullptr;  // Pal's commands also write to <data>/pal/session.log

static void out(const char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  std::fputs(buf, stdout);
  if (g_log) {
    std::fputs(buf, g_log);
    std::fflush(g_log);
  }
  std::string all = buf;
  size_t s = 0;
  while (s < all.size()) {
    size_t e = all.find('\n', s);
    if (e == std::string::npos) e = all.size();
    std::string line = all.substr(s, e - s);
    s = e + 1;
    size_t a = line.find_first_not_of(' ');
    if (a == std::string::npos) continue;
    line = line.substr(a);
    if (line.rfind("== ", 0) == 0) g_status.step = line.substr(3);
    g_status.lines.push_back(line);
    if (g_status.lines.size() > 14) g_status.lines.erase(g_status.lines.begin());
  }
  g_status.write();
}

static bool exists(const std::string &p) { return access(p.c_str(), F_OK) == 0; }

// Written after the first good reading of a boot (/tmp is emptied at boot).
static const char *kSpyderChecked = "/tmp/bloob-spyder-checked";

static TvConfig config(const std::string &mode) {
  std::string probe = env_or("BLOOB_PROBE", kProbe), data = env_or("BLOOB_CAL_DATA", kData);
  TvConfig c;
  c.meter.spotread = probe + "/spotread";
  c.meter.libs = probe + "/libs";
  c.meter.loader = env_or("BLOOB_LOADER", "/lib/ld-linux.so.3");
  if (c.meter.loader == "none") c.meter.loader.clear();
  std::string ccss = data + "/argyll/lg-55g2.ccss";
  if (exists(ccss)) c.meter.ccss = ccss;
  // spotread's display type from the correction file, as PGenerator+ chooses it: refresh
  // ("c") when it says DISPLAY_TYPE_REFRESH "YES" or names an OLED, otherwise "l"; refresh
  // for this OLED when there is no file.
  {
    std::ifstream f(c.meter.ccss);
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    bool says_no = text.find("DISPLAY_TYPE_REFRESH \"NO\"") != std::string::npos;
    c.meter.display = c.meter.ccss.empty() || !says_no ? "c" : "l";
  }
  // The Spyder5 calibrates itself on its first start after a boot; later starts reuse it.
  c.meter.no_init_cal = exists(kSpyderChecked);
  // An explicit refresh rate, once known ({"refresh_hz": 120} in meter.json).
  {
    std::ifstream f(data + "/meter.json");
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    bool ok = false;
    Json m = Json::parse(text, &ok);
    double hz = ok ? m["refresh_hz"].as_number() : 0;
    if (hz >= 20 && hz <= 480) c.meter.refresh_hz = std::to_string(static_cast<int>(hz + 0.5));
  }
  if (getenv("BLOOB_METER_REFRESH")) c.meter.refresh_hz = getenv("BLOOB_METER_REFRESH");
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

// Live progress for the screen: the reading count, and a note while a dark patch reads
// (the Spyder takes up to about 20 s on an OLED black).
static void report_progress(TvDevice &tv) {
  tv.on_reading = [](long n) {
    g_status.readings = n;
    g_status.write();
  };
  tv.on_patch = [](const Patch &p) {
    int top = std::max(p.r, std::max(p.g, p.b));
    g_status.note = top < 200 ? "Reading a dark patch: the meter takes up to 20 s" : "";
    g_status.write();
  };
}

static bool read_avg(TvDevice &tv, const Patch &p, int n, Reading *avg, double *spread = nullptr) {
  std::vector<Vec3> v;
  for (int i = 0; i < n && !g_stop; i++) {
    Vec3 x;
    auto t0 = std::chrono::steady_clock::now();
    if (!tv.measure(p, &x)) {
      out("    reading failed: %s\n", tv.last_error().c_str());
      return false;
    }
    double cx, cy;
    chromaticity(x, &cx, &cy);
    out("    %d/%d  code %d,%d,%d  Y %.3f  x %.4f  y %.4f  (%.1f s)\n", i + 1, n, p.r, p.g, p.b, x.y, cx, cy,
                std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    v.push_back(x);
  }
  if (v.empty()) return false;
  Vec3 m;
  for (auto &x : v) m = m + x / static_cast<double>(v.size());
  avg->xyz = m;
  chromaticity(m, &avg->x, &avg->y);
  if (spread) {
    double s = 0;
    for (auto &x : v) s += (x.y - m.y) * (x.y - m.y);
    *spread = v.size() > 1 && m.y > 0 ? 100 * std::sqrt(s / (v.size() - 1)) / m.y : 0;
  }
  return true;
}

static void show(const char *label, const Reading &r) {
  out("  %-34s Y %8.3f cd/m2   x %.4f  y %.4f\n", label, r.xyz.y, r.x, r.y);
}

static int pass_count = 0, fail_count = 0;
static void verdict(bool ok, const std::string &what) {
  out("  %s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  (ok ? pass_count : fail_count)++;
}

static void tables(TvDevice &tv) {
  out("== Tables (read-only)\n");
  const char *cmds[] = {"GET_1D_DPG_DATA", "GET_3D_LUT_DATA", "GET_3BY3_GAMUT_DATA"};
  const char *names[] = {"1D LUT", "3D LUT", "3x3 matrix"};
  for (int i = 0; i < 3; i++) {
    Json r;
    if (!tv.get_table(cmds[i], &r)) {
      out("  %-10s no reply (%s)\n", names[i], tv.last_error().c_str());
      continue;
    }
    out("  %-10s %s, %lld values, %s%s\n", names[i], r["returnValue"].as_bool() ? "readable" : "not readable", r["dataCount"].as_int(),
                r["dataType"].as_string("?").c_str(), r.has("errorText") ? (" (" + r["errorText"].as_string() + ")").c_str() : "");
  }
}

static bool meter_up(TvDevice &tv) {
  out("== Meter\n");
  std::string err;
  if (!tv.open_meter(&err)) {
    out("  The meter did not answer: %s\n  Is the Spyder5 plugged into the TV?\n", err.c_str());
    return false;
  }
  out("  %s\n", tv.meter_session() ? "one session open (fast readings)" : "one reading per start (slower; the session did not open)");
  out("  correction: %s\n", config("expert1").meter.ccss.empty() ? "none found (readings uncorrected)" : "LG 55G2 (i1 Pro 2)");
  return true;
}

// Do the test squares reach the screen? On the owner's TV they were first hidden under
// Bloob's page: the meter read the page (18 then 1.7 cd/m², greenish), never a square.
// A white square must be bright, near neutral and far above a black one; otherwise the
// run stops before it changes anything.
static bool squares_visible(TvDevice &tv) {
  out("== Are the squares on the screen?\n");
  Reading w, k;
  if (!read_avg(tv, {940, 940, 940}, 2, &w) || !read_avg(tv, {64, 64, 64}, 2, &k)) return false;
  bool bright = w.xyz.y >= 30, neutral = w.x > 0.26 && w.x < 0.37 && w.y > 0.27 && w.y < 0.39, contrast = w.xyz.y >= 10 * std::max(k.xyz.y, 0.05);
  if (bright && neutral && contrast) {
    out("  Yes: white %.1f cd/m2, black %.3f cd/m2.\n", w.xyz.y, k.xyz.y);
    std::ofstream(kSpyderChecked) << "1\n";  // later starts this boot skip the Spyder's own calibration (-N)
    return true;
  }
  out("  No. The meter read white %.1f cd/m2 (x %.3f y %.3f) and black %.2f cd/m2, which is not a test square.\n", w.xyz.y, w.x, w.y, k.xyz.y);
  out("  The TV draws its squares only while a video plays: start runs from Bloob's Calibrate screen, which plays a black\n");
  out("  video for them (over SSH, play something first), and keep the meter flat in the middle.\n");
  out("  If squares still do not appear, send the log: nothing was changed.\n");
  return false;
}

// Which way of drawing LG's test squares works on this TV? Changes nothing: each way draws
// a box of its own colour for 10 s and says what the TV answered; the owner says which
// colours appeared. Calibration mode (way 5) changes colour temperature and gamut on its
// own, so both are read first and put back.
static int cmd_patterns() {
  struct Way {
    const char *colour;
    Patch patch;
    bool socket, background, calibration;
    const char *how;
  };
  const Way ways[] = {
      {"RED", {940, 64, 64}, true, true, false, "second-screen socket, box on a black screen (Bloob's way)"},
      {"GREEN", {64, 940, 64}, true, false, false, "second-screen socket, box alone over what is on screen"},
      {"BLUE", {64, 64, 940}, false, true, false, "luna-send, box on a black screen"},
      {"YELLOW", {940, 940, 64}, false, false, false, "luna-send, box alone over what is on screen"},
      {"WHITE", {940, 940, 940}, true, true, true, "second-screen socket, inside calibration mode (Expert 1)"},
  };
  TvDevice tv(config("expert1"));
  Json pic;
  if (tv.read_picture(&pic)) out("== Picture now: %s\n", Json::stringify(pic).c_str());
  out("== Watch the middle of the screen. Each step shows one coloured box for 10 seconds.\n");
  out("   Note which colours you see. If the TV asks to allow Bloob, press OK.\n");
  int n = 0;
  for (const Way &w : ways) {
    if (g_stop) break;
    n++;
    out("== Step %d: %s box (%s)\n", n, w.colour, w.how);
    if (w.socket) unsetenv("BLOOB_PATTERNS_LUNA");
    else setenv("BLOOB_PATTERNS_LUNA", "1", 1);
    Json before;
    bool have_before = false;
    if (w.calibration) {
      have_before = tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &before);
      if (!tv.set_picture_mode("expert1") || !tv.set_calibration_mode(true)) {
        out("  calibration mode refused: %s\n", tv.last_error().c_str());
        continue;
      }
    }
    bool ok = tv.show_box(w.patch, w.background);
    out("  %s\n", ok ? "The TV accepted it." : ("The TV refused it: " + tv.last_error()).c_str());
    for (int i = 0; i < 10 && !g_stop; i++) sleep(1);
    if (!tv.patterns_off()) out("  turning it off: %s\n", tv.last_error().c_str());
    if (w.calibration) {
      tv.set_calibration_mode(false);
      if (have_before && !tv.set_picture_settings(Json::stringify(before))) out("  putting colour temperature and gamut back: %s\n", tv.last_error().c_str());
    }
    sleep(2);
  }
  unsetenv("BLOOB_PATTERNS_LUNA");
  tv.finish();
  out("Done. Nothing was changed. Which colours did you see: red, green, blue, yellow, white?\n");
  return 0;
}

static int cmd_check() {
  TvDevice tv(config("expert1"));
  report_progress(tv);
  Json pic;
  if (tv.read_picture(&pic)) out("== Picture now: %s\n", Json::stringify(pic).c_str());
  tables(tv);
  if (!meter_up(tv)) return 1;
  if (!squares_visible(tv)) return 1;
  out("== Readings (keep the meter still and the room dark)\n");
  out("  Showing a white square in the middle of the screen. If the TV asks to allow Bloob, press OK.\n");
  Reading w, hi;
  double spread = 0;
  auto t0 = std::chrono::steady_clock::now();
  if (!read_avg(tv, {940, 940, 940}, 10, &w, &spread)) return 1;
  double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 10;
  show("white 940 (10 readings)", w);
  out("  steadiness: %.2f%% spread, %.1f s per reading\n", spread, secs);
  if (!read_avg(tv, {1019, 1019, 1019}, 3, &hi)) return 1;
  show("white 1019 (range test)", hi);
  double ratio = hi.xyz.y / std::max(w.xyz.y, 1e-6);
  // Code 64 read as black in the square check, so the TV takes video (limited) range; a brighter
  // 1019 only means it also shows the levels above white.
  out("  range: 1019 is %.2fx 940 -> %s\n", ratio, ratio > 1.02 ? "video range, levels above white shown" : "video range, above white clipped");
  tv.finish();
  out("Done. Nothing was changed.\n");
  return 0;
}

static int cmd_test() {
  TvDevice probe(config("expert1"));
  Json pic;
  if (!probe.read_picture(&pic)) {
    out("Could not read the picture settings: %s\n", probe.last_error().c_str());
    return 1;
  }
  std::string home_mode = pic["pictureMode"].as_string("expert1");
  if (home_mode == "expert2") home_mode = "expert1";
  out("== You are in %s; the test uses expert2 and comes back to %s.\n", pic["pictureMode"].as_string("?").c_str(), home_mode.c_str());

  TvDevice tv(config("expert2"));
  report_progress(tv);
  tables(tv);
  if (!meter_up(tv)) return 1;
  if (!tv.set_picture_mode("expert2")) {
    out("Could not switch to expert2: %s\n", tv.last_error().c_str());
    return 1;
  }
  sleep(2);
  if (!squares_visible(tv)) {
    tv.finish();
    if (!tv.set_picture_mode(home_mode)) out("  Could not switch back to %s: %s\n", home_mode.c_str(), tv.last_error().c_str());
    return 1;
  }
  // Calibration mode sets its own colour temperature and gamut (Warm 50, Wide [bsc]); they
  // are put back at the end, and the "after" reading is taken outside calibration mode with
  // them back, so it compares like with like (on the owner's TV the first test compared a
  // reading inside calibration mode with one outside: 10% and 0.010 xy apart).
  Json e2;
  bool have_e2 = tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &e2);
  if (have_e2) out("  Expert 2: colour temperature %s, gamut %s\n", e2["colorTemperature"].as_string("?").c_str(), e2["colorGamut"].as_string("?").c_str());
  bool wrote = false;
  auto restore = [&]() {
    if (wrote && g_stop) {
      // Stopped part-way: Expert 2 still goes back to factory before anything else.
      int stop = g_stop;
      g_stop = 0;
      bool ok = tv.set_calibration_mode(true) && tv.reset_factory();
      out("  %s\n", ok ? "Stopped: Expert 2 put back to factory." : ("Stopped, and Expert 2 could not be reset: " + tv.last_error()).c_str());
      g_stop = stop;
    }
    tv.finish();
    if (have_e2 && !tv.set_picture_settings(Json::stringify(e2))) out("  Could not put Expert 2's colour temperature and gamut back: %s\n", tv.last_error().c_str());
    if (!tv.set_picture_mode(home_mode)) out("  Could not switch back to %s: %s\n", home_mode.c_str(), tv.last_error().c_str());
    else out("  Back in %s.\n", home_mode.c_str());
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
    out("== Saved Expert 2's 1D LUT to %s/expert2-%s-1d.json\n", dir.c_str(), stamp);
  }

  out("== Before (Expert 2 as it is)\n");
  Reading w0, r0, g0;
  if (!read_avg(tv, {940, 940, 940}, 2, &w0) || !read_avg(tv, {721, 64, 64}, 2, &r0) || !read_avg(tv, {502, 502, 502}, 2, &g0)) {
    restore();
    return 1;
  }
  show("white", w0);
  show("red 75%", r0);
  show("grey 50%", g0);

  out("== Writing test tables (Expert 2 only)\n");
  if (!tv.set_calibration_mode(true)) {
    out("  Calibration mode refused: %s\n", tv.last_error().c_str());
    restore();
    return 1;
  }
  verdict(true, "calibration mode started for expert2");
  verdict(tv.calman_bypass(), "gamma switches off and the BT.709 matrix at identity, as Calman starts (factory reset puts them back)");
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
  out("== Pipeline order (needs the 3D LUT, sent over the TV's socket; the TV may ask to allow Bloob)\n");
  Lut3D swap = Lut3D::unity();
  for (auto &n : swap.node) std::swap(n.x, n.y);
  Lut1D blank = Lut1D::unity();
  for (double &v : blank.ch[0]) v = 0;
  if (tv.upload_3d(Slot::Bt709, swap) && tv.upload_1d(blank)) {
    Reading o;
    if (read_avg(tv, {721, 64, 64}, 2, &o)) {
      show("red input, swap + blank", o);
      bool green = o.xyz.y > 0.5 && o.y > 0.5;
      out("  order: %s\n", o.xyz.y < 0.5 ? "1D LUT first, then 3D LUT" : green ? "3D LUT first, then 1D LUT (the expected order)" : "unclear");
      verdict(o.xyz.y < 0.5 || green, "the TV applies 3D tables, and the order is known");
    }
    tv.upload_3d(Slot::Bt709, Lut3D::unity());
  } else {
    verdict(false, "3D LUT: " + tv.last_error());
  }

  out("== Back to factory (Expert 2)\n");
  ok = tv.reset_factory();
  verdict(ok, "factory reset accepted" + (ok ? std::string() : ": " + tv.last_error()));
  if (have_backup) {
    Json after;
    if (tv.get_table("GET_1D_DPG_DATA", &after))
      verdict(after["data"].as_string() == before1d["data"].as_string(), "the 1D LUT read back after the reset equals the saved one");
  }
  // Out of calibration mode, colour temperature and gamut back, then the comparison.
  tv.set_calibration_mode(false);
  Json left;
  if (tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &left))
    out("  After calibration mode: colour temperature %s, gamut %s\n", left["colorTemperature"].as_string("?").c_str(), left["colorGamut"].as_string("?").c_str());
  if (have_e2 && !tv.set_picture_settings(Json::stringify(e2))) out("  Could not put colour temperature and gamut back: %s\n", tv.last_error().c_str());
  sleep(1);
  Reading w3, r3;
  if (ok && read_avg(tv, {940, 940, 940}, 2, &w3) && read_avg(tv, {721, 64, 64}, 2, &r3)) {
    show("white after reset", w3);
    show("red after reset", r3);
    double dw = std::fabs(w3.xyz.y / w0.xyz.y - 1) * 100;
    double dxy = std::hypot(w3.x - w0.x, w3.y - w0.y);
    verdict(dw < 2 && dxy < 0.003, "Expert 2 is back to how it was (white " + std::to_string(dw).substr(0, 4) + "% and " +
                                       std::to_string(dxy).substr(0, 6) + " xy away)");
  }
  restore();
  out("== %d passed, %d failed. Expert 1 was not touched.\n", pass_count, fail_count);
  return fail_count ? 2 : 0;
}

// ---- Expert 1: calibrate and undo ----------------------------------------

static const char *kSettingKeys[] = {"backlight", "contrast", "brightness", "color", "gamma", "colorGamut", "colorTemperature"};

// The settings as read, minus the picture mode, for setSystemSettings.
static std::string settings_payload(const Json &pic) {
  std::string j = "{";
  bool first = true;
  for (const char *k : kSettingKeys) {
    if (!pic.has(k)) continue;
    j += (first ? "" : ",") + Json::quote(k) + ":" + Json::stringify(pic[k]);
    first = false;
  }
  return j + "}";
}

// Picture processing that moves the readings (CALIBRATION-PLAN.md §7.1 step 4), with its off
// value. Names and values from bscpylgtv's list for the C2, the G2's generation; only the keys
// this TV reports are touched. They stay off after a calibration (it measured the picture
// without them); Undo and a failed run put the first run's values back.
struct Processing {
  const char *category, *key, *off;
};
static const Processing kProcessing[] = {
    {"picture", "energySaving", "off"},      {"picture", "dynamicContrast", "off"},          {"picture", "dynamicColor", "off"},
    {"picture", "peakBrightness", "off"},    {"picture", "logoLuminanceAdjust", "off"},      {"picture", "ambientLightCompensation", "off"},
    {"aiPicture", "ai_Picture", "off"},      {"aiPicture", "ai_Brightness", "off"},          {"aiPicture", "ai_Genre", "off"},
};
static const char *kProcessingCategories[] = {"picture", "aiPicture"};

static std::vector<std::string> processing_keys(const char *category) {
  std::vector<std::string> keys;
  for (const Processing &p : kProcessing)
    if (std::string(p.category) == category) keys.push_back(p.key);
  return keys;
}

// {"picture":{...},"aiPicture":{...}}, with the keys this TV has.
static std::string read_processing(TvDevice &tv) {
  std::string j = "{";
  for (const char *c : kProcessingCategories) {
    Json now;
    if (!tv.read_settings(c, processing_keys(c), &now)) now = Json::parse("{}");
    j += std::string(j.size() > 1 ? "," : "") + Json::quote(c) + ":" + Json::stringify(now);
  }
  return j + "}";
}

// Switches off what is on; says what it changed.
static void processing_off(TvDevice &tv, const Json &now) {
  int changed = 0;
  for (const char *c : kProcessingCategories) {
    std::string payload = "{";
    for (const Processing &p : kProcessing) {
      if (std::string(p.category) != c || !now[c].has(p.key)) continue;
      std::string was = now[c][p.key].as_string();
      if (was == p.off) continue;
      payload += std::string(payload.size() > 1 ? "," : "") + Json::quote(p.key) + ":" + Json::quote(p.off);
      out("  %s: %s -> %s\n", p.key, was.c_str(), p.off);
      changed++;
    }
    if (payload.size() > 1 && !tv.set_settings(c, payload + "}")) out("  The TV refused some of these: %s\n", tv.last_error().c_str());
  }
  if (!changed) out("  Picture processing already off.\n");
}

static bool restore_processing(TvDevice &tv) {
  std::ifstream f(data_dir() + "/expert1-processing.json");
  std::string saved((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  bool have = false;
  Json was = Json::parse(saved, &have);
  if (!have) return false;
  bool ok = true;
  for (const char *c : kProcessingCategories) {
    std::string payload = "{";
    for (const Processing &p : kProcessing)
      if (std::string(p.category) == c && was[c].has(p.key))
        payload += std::string(payload.size() > 1 ? "," : "") + Json::quote(p.key) + ":" + Json::stringify(was[c][p.key]);
    if (payload.size() > 1) ok = tv.set_settings(c, payload + "}") && ok;
  }
  return ok;
}

// Factory data for Expert 1 and the saved settings back (also after a failed or stopped run).
static bool undo_expert1(TvDevice &tv, std::string *note) {
  int stop = g_stop;
  g_stop = 0;
  bool ok = tv.set_picture_mode("expert1");
  sleep(1);
  ok = ok && tv.set_calibration_mode(true) && tv.reset_factory();
  tv.finish();
  std::ifstream f(data_dir() + "/expert1-settings.json");
  std::string saved((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  bool have = false;
  Json pic = Json::parse(saved, &have);
  if (have && tv.set_picture_settings(settings_payload(pic))) *note = "factory tables and your saved settings";
  else *note = have ? "factory tables (your settings could not be put back: " + tv.last_error() + ")" : "factory tables (no saved settings)";
  if (restore_processing(tv)) *note += ", picture processing as it was";
  g_stop = stop;
  return ok;
}

static std::string stats_json(const Stats &s) {
  char b[200];
  std::snprintf(b, sizeof b, "{\"grey_avg\":%.2f,\"grey_max\":%.2f,\"colour_avg\":%.2f,\"colour_max\":%.2f}", s.grey_avg, s.grey_max, s.colour_avg,
                s.colour_max);
  return b;
}

// After calibration mode: the same colours, as you will watch them.
static bool viewing_check(TvDevice &tv, const SdrTarget &t, Stats *s) {
  const double greys[] = {0.25, 0.5, 0.75, 1.0};
  const Vec3 colours[] = {{0.75, 0, 0}, {0, 0.75, 0}, {0, 0, 0.75}, {0, 0.75, 0.75}, {0.75, 0, 0.75}, {0.75, 0.75, 0}};
  Vec3 white = t.white();
  auto one = [&](Vec3 sig, bool grey) {
    Patch p{code_limited(sig.x), code_limited(sig.y), code_limited(sig.z)};
    Reading r;
    if (!read_avg(tv, p, 2, &r)) return false;
    double de = de2000_xyz(r.xyz, t.xyz({signal_limited(p.r), signal_limited(p.g), signal_limited(p.b)}), white);
    (grey ? s->grey_avg : s->colour_avg) += de;
    double &mx = grey ? s->grey_max : s->colour_max;
    mx = std::max(mx, de);
    (grey ? s->grey_n : s->colour_n)++;
    return true;
  };
  for (double g : greys)
    if (!one({g, g, g}, true)) return false;
  for (const Vec3 &c : colours)
    if (!one(c, false)) return false;
  s->grey_avg /= s->grey_n;
  s->colour_avg /= s->colour_n;
  return true;
}

static int cmd_sdr(double nits, bool quick) {
  TvDevice tv(config("expert1"));
  Json pic;
  if (!tv.read_picture(&pic)) {
    out("Could not read the picture settings: %s\n", tv.last_error().c_str());
    return 1;
  }
  std::string mode = pic["pictureMode"].as_string("?");
  out("== Getting ready (calibrating Expert 1 to %.0f nits)\n", nits);
  // Your settings before the first change, for Undo (kept from the first run until Undo).
  std::string saved = data_dir() + "/expert1-settings.json";
  if (!exists(saved)) {
    if (mode != "expert1") tv.set_picture_mode("expert1"), sleep(1), tv.read_picture(&pic);
    std::ofstream(saved) << Json::stringify(pic);
    out("  Saved your Expert 1 settings for Undo.\n");
  }
  std::string saved_processing = data_dir() + "/expert1-processing.json";
  if (!tv.set_picture_mode("expert1")) {
    out("Could not switch to Expert 1: %s\n", tv.last_error().c_str());
    return 1;
  }
  sleep(1);
  if (tv.read_picture(&pic)) tv.set_light_cache(static_cast<int>(pic["backlight"].as_number(std::atof(pic["backlight"].as_string("28").c_str()))));
  tables(tv);
  if (!meter_up(tv)) return 1;
  if (!squares_visible(tv)) return 1;
  report_progress(tv);
  g_log_hook = [](const std::string &line) { out("  %s\n", line.c_str()); };

  // Measured without the picture processing that moves the readings (Calman sets these too).
  out("== Picture processing off for the measurements\n");
  std::string processing = read_processing(tv);
  if (!exists(saved_processing)) std::ofstream(saved_processing) << processing;
  processing_off(tv, Json::parse(processing));
  if (!tv.set_calibration_mode(true)) {
    out("Calibration mode refused: %s\n", tv.last_error().c_str());
    restore_processing(tv);
    return 1;
  }
  // From LG's factory calibration data, as Undo leaves it: matrices and gamma switches too,
  // not only the two tables the procedure sets to neutral.
  out("== Starting from LG's factory calibration data\n");
  if (!tv.reset_factory()) {
    out("The factory reset was refused: %s\n", tv.last_error().c_str());
    tv.finish();
    restore_processing(tv);
    return 1;
  }
  // Then as Calman starts SDR: gamma switches off, BT.709 matrix at identity, contrast 85,
  // brightness 50, colour 50 (research/PGenerator-Plus-and-LG-for-Bloob.md L3, L4, L6).
  out("  Gamma switches off, colour matrix neutral, contrast 85, brightness 50, colour 50 (as Calman starts).\n");
  if (!tv.start_like_calman()) {
    out("The TV refused Calman's start: %s\n", tv.last_error().c_str());
    tv.finish();
    std::string note;
    undo_expert1(tv, &note);
    return 1;
  }
  // Calibration mode sets its own colour temperature and gamut (Warm 50, Wide [bsc]); the
  // tables are measured in that state, so it is the state to watch in afterwards.
  Json during;
  bool have_during = tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &during);
  if (have_during) out("  In calibration mode: colour temperature %s, gamut %s\n", during["colorTemperature"].as_string("?").c_str(),
                       during["colorGamut"].as_string("?").c_str());
  out(quick ? "== Calibrating, quick (leave the meter on the screen; about 1½ to 2 hours)\n"
            : "== Calibrating, thorough (leave the meter on the screen; this takes a few hours)\n");
  SdrOptions o = quick ? SdrOptions::quick() : SdrOptions::tv_profile();
  o.target.Lw = nits;
  auto t0 = std::chrono::steady_clock::now();
  SdrResult r = run_sdr(tv, o);
  double minutes = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 60;
  tv.finish();

  if (!r.ok || g_stop) {
    std::string note;
    bool undone = undo_expert1(tv, &note);
    std::string why = g_stop ? "stopped" : r.error;
    if (g_stop) out("== Calibration stopped\n");
    else out("== Calibration did not finish: %s\n", why.c_str());
    out("  Expert 1 is back to %s%s.\n", note.c_str(), undone ? "" : " (the factory reset was refused; use Undo)");
    g_status.result = "{\"ok\":false,\"error\":" + Json::quote(why) + ",\"undone\":" + (undone ? "true" : "false") + ",\"before\":" +
                      stats_json(r.baseline) + "}";
    return 2;
  }
  // Save the tables written, and check the result in the viewing state.
  char stamp[32];
  time_t now = time(nullptr);
  strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", localtime(&now));
  std::string runs = data_dir() + "/runs";
  mkdir(runs.c_str(), 0755);
  std::ofstream(runs + "/expert1-" + stamp + ".cube") << to_cube(r.lut3d, "Bloob Expert 1");
  if (have_during) {
    Json after;
    std::string payload = "{";
    if (tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &after))
      for (const char *k : {"colorTemperature", "colorGamut"})
        if (during.has(k) && after.has(k) && Json::stringify(after[k]) != Json::stringify(during[k])) {
          out("  %s went back to %s after calibration mode: set to %s, as measured\n", k, after[k].as_string("?").c_str(), during[k].as_string("?").c_str());
          payload += std::string(payload.size() > 1 ? "," : "") + Json::quote(k) + ":" + Json::stringify(during[k]);
        }
    if (payload.size() > 1 && !tv.set_picture_settings(payload + "}")) out("  The TV refused: %s\n", tv.last_error().c_str());
  }
  out("== Checking the result as you will watch it\n");
  Stats view;
  bool viewed = viewing_check(tv, o.target, &view);
  tv.finish();
  out("== Done\n");
  out("  Before: grey %.2f, colour %.2f dE.  After: grey %.2f, colour %.2f dE (worst %.2f / %.2f).\n", r.baseline.grey_avg, r.baseline.colour_avg,
      r.final.grey_avg, r.final.colour_avg, r.final.grey_max, r.final.colour_max);
  if (viewed) out("  As you watch it: grey %.2f, colour %.2f dE.\n", view.grey_avg, view.colour_avg);
  char extra[200];
  std::snprintf(extra, sizeof extra, ",\"light\":%d,\"readings\":%ld,\"minutes\":%.0f,\"nits\":%.0f,\"order\":\"%s\",\"profile\":\"%s\"", r.light, tv.readings(), minutes,
                nits, r.lut1d_first ? "1D then 3D" : "3D then 1D", quick ? "quick" : "thorough");
  g_status.result = "{\"ok\":true,\"before\":" + stats_json(r.baseline) + ",\"after\":" + stats_json(r.final) +
                    (viewed ? ",\"viewing\":" + stats_json(view) : std::string()) + extra + "}";
  return 0;
}

static int cmd_undo() {
  TvDevice tv(config("expert1"));
  out("== Undo: Expert 1 back to factory\n");
  std::string note;
  bool ok = undo_expert1(tv, &note);
  out("  %s: %s.\n", ok ? "Done" : "The factory reset was refused", note.c_str());
  g_status.result = std::string("{\"ok\":") + (ok ? "true" : "false") + "}";
  return ok ? 0 : 2;
}


// ---- Pal's grey flow (research/Pal-in-Bloob.md §9–11, native/PAL-PLAYBOOK.md) ----------
// Pal (Claude, on the owner's computer over SSH) runs these one at a time while Bloob's
// Calibrate screen holds a Pal session (the black video LG's squares need). The session lives
// in <data>/pal/: session.json, every report, proposal and table. Each command prints its
// result as one line, "PAL {json}", and exits 0 when it did what was asked; 4 when the gate
// refused a table, 5 when the TV did not load it, 6 when a pre-flight check failed, 2 otherwise.

static std::string pal_dir() { return data_dir() + "/pal"; }

static std::string slurp_file(const std::string &path) {
  std::ifstream f(path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void write_file(const std::string &path, const std::string &text) {
  std::string tmp = path + ".tmp";
  { std::ofstream(tmp) << text; }
  ::rename(tmp.c_str(), path.c_str());
}

static std::string stamp_now() {
  char b[32];
  time_t now = time(nullptr);
  strftime(b, sizeof b, "%Y%m%d-%H%M%S", localtime(&now));
  return b;
}

static std::string num(double v, int places = 4) {
  if (!std::isfinite(v)) return "null";
  char b[48];
  std::snprintf(b, sizeof b, "%.*f", places, v);
  return b;
}

static std::string vec(const Vec3 &v, int places = 4) { return "[" + num(v.x, places) + "," + num(v.y, places) + "," + num(v.z, places) + "]"; }

static std::string list(const std::vector<double> &v, int places = 5) {
  std::string j = "[";
  for (size_t i = 0; i < v.size(); i++) j += (i ? "," : "") + num(v[i], places);
  return j + "]";
}

// The result line Pal reads (written whole, not through out(), which is for short lines).
static int pal_result(int rc, const std::string &json) {
  g_status.result = json;
  std::string line = "PAL " + json + "\n";
  std::fputs(line.c_str(), stdout);
  std::fflush(stdout);
  if (g_log) {
    std::fputs(line.c_str(), g_log);
    std::fflush(g_log);
  }
  return rc;
}

static int pal_fail(int rc, const std::string &why, const std::string &extra = "") {
  out("  %s\n", why.c_str());
  return pal_result(rc, "{\"ok\":false,\"error\":" + Json::quote(why) + extra + "}");
}

static std::vector<uint16_t> u16_from_base64(const std::string &b64) {
  std::vector<unsigned char> bytes;
  unsigned v = 0;
  int bits = 0;
  for (char ch : b64) {
    int d = ch >= 'A' && ch <= 'Z' ? ch - 'A' : ch >= 'a' && ch <= 'z' ? ch - 'a' + 26 : ch >= '0' && ch <= '9' ? ch - '0' + 52 : ch == '+' ? 62 : ch == '/' ? 63 : -1;
    if (d < 0) continue;
    v = v << 6 | static_cast<unsigned>(d);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      bytes.push_back(static_cast<unsigned char>(v >> bits & 0xff));
    }
  }
  std::vector<uint16_t> out16;
  for (size_t i = 0; i + 1 < bytes.size(); i += 2) out16.push_back(static_cast<uint16_t>(bytes[i] | bytes[i + 1] << 8));
  return out16;
}

static std::string hex64(uint64_t h) {
  char b[20];
  std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(h));
  return b;
}

// The 1D table in the TV now (readable outside calibration mode on the owner's TV).
static bool read_1d(TvDevice &tv, Lut1D *t, std::string *fnv) {
  Json r;
  if (!tv.get_table("GET_1D_DPG_DATA", &r) || !r["returnValue"].as_bool()) return false;
  auto v = u16_from_base64(r["data"].as_string());
  if (v.size() != 3 * Lut1D::kSize) return false;
  *t = Lut1D::decode(v);
  if (fnv) *fnv = hex64(fnv1a(v));
  return true;
}

struct PalSession {
  bool open = false;
  double nits = 140, headroom = 1.04;
  TableIndex index = TableIndex::Signal;
  double wobble_white = 0.3, wobble_mid = 0.3;
  int light = -1, reports = 0, proposals = 0, applied = 0;
  std::string view_ct = "null", view_gamut = "null";  // as JSON values, as the TV reported them
  std::string started, start_fnv;
};

static bool load_session(PalSession *s) {
  bool ok = false;
  Json j = Json::parse(slurp_file(pal_dir() + "/session.json"), &ok);
  if (!ok || !j["open"].as_bool()) return false;
  s->open = true;
  s->nits = j["nits"].as_number(140);
  s->headroom = j["headroom"].as_number(1.04);
  s->index = j["index"].as_string() == "code" ? TableIndex::Code : TableIndex::Signal;
  s->wobble_white = j["wobble"]["white_pct"].as_number(0.3);
  s->wobble_mid = j["wobble"]["mid_pct"].as_number(0.3);
  s->light = static_cast<int>(j["light"].as_int(-1));
  s->reports = static_cast<int>(j["reports"].as_int());
  s->proposals = static_cast<int>(j["proposals"].as_int());
  s->applied = static_cast<int>(j["applied"].as_int());
  s->view_ct = Json::stringify(j["viewing"]["colorTemperature"]);
  s->view_gamut = Json::stringify(j["viewing"]["colorGamut"]);
  s->started = j["started"].as_string();
  s->start_fnv = j["start_table"].as_string();
  return true;
}

static void save_session(const PalSession &s) {
  write_file(pal_dir() + "/session.json",
             "{\"open\":" + std::string(s.open ? "true" : "false") + ",\"nits\":" + num(s.nits, 1) + ",\"headroom\":" + num(s.headroom, 3) +
                 ",\"eotf\":\"bt1886\",\"index\":\"" + (s.index == TableIndex::Code ? "code" : "signal") + "\",\"wobble\":{\"white_pct\":" +
                 num(s.wobble_white, 3) + ",\"mid_pct\":" + num(s.wobble_mid, 3) + "},\"light\":" + std::to_string(s.light) +
                 ",\"reports\":" + std::to_string(s.reports) + ",\"proposals\":" + std::to_string(s.proposals) + ",\"applied\":" +
                 std::to_string(s.applied) + ",\"viewing\":{\"colorTemperature\":" + s.view_ct + ",\"colorGamut\":" + s.view_gamut +
                 "},\"started\":" + Json::quote(s.started) + ",\"start_table\":" + Json::quote(s.start_fnv) + "}");
}

static SdrTarget pal_sdr_target(const PalSession &s) {
  SdrTarget t;
  t.Lw = s.nits;
  t.Lb = 0;  // OLED black: BT.1886 becomes a 2.4 power curve
  return t;
}

// Calibration mode changes colour temperature and gamut on its own (Warm 50 and Wide on the
// owner's TV, 3.3.7). Pal measures as the owner watches (B3), so both go back after every write.
static bool viewing_state(TvDevice &tv, const PalSession &s, std::string *note) {
  Json now;
  if (!tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &now)) {
    *note = "could not read colour temperature and gamut: " + tv.last_error();
    return false;
  }
  if (Json::stringify(now["colorTemperature"]) == s.view_ct && Json::stringify(now["colorGamut"]) == s.view_gamut) return true;
  if (!tv.set_picture_settings("{\"colorTemperature\":" + s.view_ct + ",\"colorGamut\":" + s.view_gamut + "}")) {
    *note = "could not put colour temperature and gamut back: " + tv.last_error();
    return false;
  }
  Json again;
  bool ok = tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &again) && Json::stringify(again["colorTemperature"]) == s.view_ct &&
            Json::stringify(again["colorGamut"]) == s.view_gamut;
  *note = ok ? "colour temperature and gamut put back to the viewing values" : "colour temperature and gamut did not read back as set";
  return ok;
}

// A write inside calibration mode, which always ends and puts the viewing state back.
static bool pal_write(TvDevice &tv, const PalSession &s, const std::function<bool()> &fn, std::string *err) {
  bool ok = tv.set_calibration_mode(true) && fn();
  if (!ok) *err = tv.last_error();
  tv.set_calibration_mode(false);
  tv.patterns_off();
  std::string note;
  if (!viewing_state(tv, s, &note)) {
    if (ok) *err = note;
    return false;
  }
  if (!note.empty()) out("  %s\n", note.c_str());
  return ok;
}

// Readings agree when their luminance is within the meter's wobble (or 0.01 cd/m²) and, once
// bright enough to judge, their colour within 0.002 u′v′.
static bool readings_agree(const Vec3 &a, const Vec3 &b, double wobble_pct) {
  double m = std::max(1e-6, (a.y + b.y) / 2), dy = std::fabs(a.y - b.y);
  if (dy > 0.01 && dy / m * 100 > std::max(0.5, 3 * wobble_pct)) return false;
  return m < 1 || duv(a, b) <= 0.002;
}

// One grey level: two readings, a third when they disagree, and the sanity check (B1: a pink
// flicker or a wrong patch is read again once it has passed).
static bool pal_read_level(TvDevice &tv, int reads, double wobble, const Vec3 *below, GreyLevel *l) {
  Patch p{l->code, l->code, l->code};
  auto take = [&](std::vector<Vec3> *v, int n) {
    for (int i = 0; i < n; i++) {
      Vec3 x;
      if (!tv.measure(p, &x)) {
        out("    reading failed: %s\n", tv.last_error().c_str());
        return false;
      }
      v->push_back(x);
    }
    return true;
  };
  auto mean_of = [](const std::vector<Vec3> &v) {
    Vec3 m;
    for (auto &x : v) m = m + x / static_cast<double>(v.size());
    return m;
  };
  std::vector<Vec3> v;
  if (!take(&v, reads)) return false;
  int count = reads;
  if (v.size() >= 2 && !readings_agree(v[0], v[1], wobble)) {
    if (!take(&v, 1)) return false;
    count++;
    size_t a = 0, b = 1;
    double best = 1e300;
    for (size_t i = 0; i < v.size(); i++)
      for (size_t j = i + 1; j < v.size(); j++) {
        double d = std::fabs(v[i].y - v[j].y);
        if (d < best) best = d, a = i, b = j;
      }
    if (!readings_agree(v[a], v[b], wobble)) {
      l->suspect = true;
      l->note = "readings disagreed";
    }
    v = {v[a], v[b]};
  }
  Vec3 m = mean_of(v);
  std::string why;
  for (int retry = 0; retry < 2 && !pal_grey_sane(m, below, wobble, &why); retry++) {
    out("    %d%%: %s; reading again\n", l->pct, why.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(getenv("BLOOB_CAL_FAST") ? 0 : tv.pattern_delay()));
    std::vector<Vec3> again;
    if (!take(&again, std::max(reads, 1))) return false;
    count += std::max(reads, 1);
    m = mean_of(again);
  }
  if (!pal_grey_sane(m, below, wobble, &why)) {
    l->suspect = true;
    l->note = why;
  }
  l->XYZ = m;
  l->measured = true;
  l->reads = count;
  double x, y;
  chromaticity(m, &x, &y);
  out("    %3d%%  Y %8.3f  x %.4f  y %.4f%s\n", l->pct, m.y, x, y, l->suspect ? ("  (" + l->note + ")").c_str() : "");
  return true;
}

struct PalReport {
  std::vector<GreyLevel> levels;
  Vec3 prim[3];
  GreySummary sum;
  Lut1D table = Lut1D::unity();
  std::string fnv, json;
};

// A grey report as the owner watches: 5–100% in 5% steps (0% is OLED black and is not read),
// the three primaries at 100%, white again at the end to catch drift.
static bool pal_report(TvDevice &tv, PalSession &s, PalReport *r, std::string *err) {
  auto t0 = std::chrono::steady_clock::now();
  long r0 = tv.readings();
  std::string note;
  if (!viewing_state(tv, s, &note)) {
    *err = note;
    return false;
  }
  if (!note.empty()) out("  %s\n", note.c_str());
  if (!read_1d(tv, &r->table, &r->fnv)) {
    *err = "could not read the 1D table: " + tv.last_error();
    return false;
  }
  int reads = getenv("BLOOB_PAL_READS") ? std::max(1, atoi(getenv("BLOOB_PAL_READS"))) : 2;
  const Vec3 *below = nullptr;
  for (int pct : pal_levels()) {
    GreyLevel l;
    l.pct = pct;
    l.code = pal_code(pct);
    if (pct > 0) {
      g_status.note = pct <= 10 ? "Reading a dark patch: the meter takes up to 20 s" : "";
      if (!pal_read_level(tv, reads, pct >= 50 ? s.wobble_white : s.wobble_mid, below, &l)) {
        *err = "the meter failed: " + tv.last_error();
        return false;
      }
    }
    r->levels.push_back(l);
    if (l.measured) below = &r->levels.back().XYZ;
  }
  g_status.note.clear();
  // The meter must still see the squares: white far below the target means it moved, fell or
  // the video under the squares stopped.
  if (r->levels.back().XYZ.y < 0.3 * s.nits) {
    *err = "White read " + num(r->levels.back().XYZ.y, 1) + " cd/m2: the meter no longer sees the squares (moved, fallen, or the video under them stopped).";
    return false;
  }
  const Patch ps[3] = {{940, 64, 64}, {64, 940, 64}, {64, 64, 940}};
  for (int c = 0; c < 3; c++)
    if (!tv.measure(ps[c], &r->prim[c])) {
      *err = "the meter failed: " + tv.last_error();
      return false;
    }
  Vec3 w_end;
  if (!tv.measure({940, 940, 940}, &w_end)) {
    *err = "the meter failed: " + tv.last_error();
    return false;
  }
  SdrTarget t = pal_sdr_target(s);
  r->sum = pal_summarise(r->levels, t, PalGoal{});
  double drift = r->sum.white_Y > 0 ? 100 * (w_end.y / r->sum.white_Y - 1) : 0;
  auto pts = pal_table_points(r->table, s.index);
  Json view;
  tv.read_settings("picture", {"colorTemperature", "colorGamut"}, &view);
  s.reports++;
  std::string j = "{\"ok\":true,\"kind\":\"report\",\"n\":" + std::to_string(s.reports) + ",\"time\":" + Json::quote(stamp_now()) +
                  ",\"in_calibration_mode\":false,\"viewing\":" + Json::stringify(view) + ",\"target\":{\"white\":" + num(s.nits, 1) +
                  ",\"eotf\":\"bt1886\",\"black\":0,\"x\":0.3127,\"y\":0.3290},\"goal\":{\"avg\":0.5,\"max\":1.0,\"white_pct\":2,\"first_scored\":10}" +
                  ",\"index\":\"" + (s.index == TableIndex::Code ? "code" : "signal") + "\",\"light\":" + std::to_string(s.light) +
                  ",\"wobble\":{\"white_pct\":" + num(s.wobble_white, 3) + ",\"mid_pct\":" + num(s.wobble_mid, 3) + "},\"table\":{\"fnv\":\"" + r->fnv +
                  "\",\"points\":{\"r\":" + list(pts[0]) + ",\"g\":" + list(pts[1]) + ",\"b\":" + list(pts[2]) + "}},\"primaries\":{\"r\":" +
                  vec(r->prim[0]) + ",\"g\":" + vec(r->prim[1]) + ",\"b\":" + vec(r->prim[2]) + "},\"levels\":[";
  for (size_t i = 0; i < r->levels.size(); i++) {
    const GreyLevel &l = r->levels[i];
    double x = 0, y = 0;
    if (l.measured) chromaticity(l.XYZ, &x, &y);
    j += std::string(i ? "," : "") + "{\"pct\":" + std::to_string(l.pct) + ",\"code\":" + std::to_string(l.code) + ",\"measured\":" +
         (l.measured ? "true" : "false") + ",\"XYZ\":" + vec(l.XYZ) + ",\"Y\":" + num(l.XYZ.y) + ",\"x\":" + num(x) + ",\"y\":" + num(y) +
         ",\"target_Y\":" + num(l.target.y) + ",\"de\":" + num(l.de, 3) + ",\"scored\":" + (l.scored ? "true" : "false") + ",\"reads\":" +
         std::to_string(l.reads) + ",\"suspect\":" + (l.suspect ? "true" : "false") + ",\"note\":" + Json::quote(l.note) + "}";
  }
  double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  j += "],\"summary\":{\"avg\":" + num(r->sum.avg, 3) + ",\"max\":" + num(r->sum.max, 3) + ",\"max_at\":" + std::to_string(r->sum.max_at) +
       ",\"white_Y\":" + num(r->sum.white_Y, 3) + ",\"white_err_pct\":" + num(r->sum.white_err_pct, 2) + ",\"goal_met\":" +
       (r->sum.goal_met ? "true" : "false") + "},\"drift_pct\":" + num(drift, 2) + ",\"readings\":" + std::to_string(tv.readings() - r0) +
       ",\"seconds\":" + num(secs, 0) + "}";
  r->json = j;
  write_file(pal_dir() + "/report-" + std::to_string(s.reports) + ".json", j);
  write_file(pal_dir() + "/latest-report.json", j);
  save_session(s);
  out("  Report %d: average %.2f, worst %.2f at %d%%, white %.1f cd/m2 (%+.1f%%)%s\n", s.reports, r->sum.avg, r->sum.max, r->sum.max_at, r->sum.white_Y,
      r->sum.white_err_pct, r->sum.goal_met ? ", goal met" : "");
  if (std::fabs(drift) > std::max(1.0, 3 * s.wobble_white)) out("  White moved %.1f%% during the report: the panel or the meter is not steady\n", drift);
  return true;
}

// A report back from its file, for pal-propose and the never-worse check.
static bool load_report(const std::string &path, std::vector<GreyLevel> *levels, Vec3 prim[3], std::string *fnv) {
  bool ok = false;
  Json j = Json::parse(slurp_file(path), &ok);
  if (!ok || j["kind"].as_string() != "report") return false;
  levels->clear();
  for (size_t i = 0; i < j["levels"].size(); i++) {
    const Json &e = j["levels"][i];
    GreyLevel l;
    l.pct = static_cast<int>(e["pct"].as_int());
    l.code = static_cast<int>(e["code"].as_int());
    l.measured = e["measured"].as_bool();
    l.XYZ = {e["XYZ"][0].as_number(), e["XYZ"][1].as_number(), e["XYZ"][2].as_number()};
    l.de = e["de"].as_number();
    l.scored = e["scored"].as_bool();
    levels->push_back(l);
  }
  const char *names[3] = {"r", "g", "b"};
  for (int c = 0; c < 3; c++) prim[c] = {j["primaries"][names[c]][0].as_number(), j["primaries"][names[c]][1].as_number(), j["primaries"][names[c]][2].as_number()};
  if (fnv) *fnv = j["table"]["fnv"].as_string();
  return !levels->empty();
}

// Light so that white reads `goal` (cd/m²) as the owner watches.
static bool pal_dial_light(TvDevice &tv, PalSession &s, double goal, std::string *err) {
  Json pic;
  if (s.light < 0 && tv.read_picture(&pic)) s.light = static_cast<int>(pic["backlight"].as_number(std::atof(pic["backlight"].as_string("28").c_str())));
  tv.set_light_cache(s.light);
  for (int i = 0; i < 6; i++) {
    Reading w;
    if (!read_avg(tv, {940, 940, 940}, 2, &w)) {
      *err = "the meter failed: " + tv.last_error();
      return false;
    }
    double off = 100 * (w.xyz.y / goal - 1);
    out("  light %d: white %.1f cd/m2 (%+.1f%% from %.1f)\n", s.light, w.xyz.y, off, goal);
    if (std::fabs(off) <= 1.5) return true;
    int next = std::clamp(static_cast<int>(std::lround(s.light * goal / std::max(w.xyz.y, 0.5))), 1, 100);
    if (next == s.light) next += off < 0 ? 1 : -1;
    if (next < 1 || next > 100) break;
    if (!pal_write(tv, s, [&] { return tv.set_light(next); }, err)) return false;
    s.light = next;
    save_session(s);
  }
  return true;  // as close as whole light steps get; the report says how close
}

static int cmd_pal_start(double nits) {
  mkdir(pal_dir().c_str(), 0755);
  PalSession old;
  if (load_session(&old)) return pal_fail(6, "A Pal session is already open: end it first (pal-end ok, or pal-end undo).");
  TvDevice tv(config("expert1"));
  report_progress(tv);
  PalSession s;
  s.nits = nits;
  s.started = stamp_now();
  out("== Pal session: getting the TV ready (Expert 1, %.0f nits)\n", nits);
  // 1. Expert 1, with the owner's settings saved for Undo.
  Json pic;
  if (!tv.read_picture(&pic)) return pal_fail(6, "Could not read the picture settings: " + tv.last_error());
  std::string saved = data_dir() + "/expert1-settings.json";
  if (pic["pictureMode"].as_string() != "expert1") {
    if (!tv.set_picture_mode("expert1")) return pal_fail(6, "Could not switch to Expert 1: " + tv.last_error());
    sleep(1);
    tv.read_picture(&pic);
  }
  if (pic["pictureMode"].as_string() != "expert1") return pal_fail(6, "Expert 1 did not become the active picture mode.");
  if (!exists(saved)) {
    std::ofstream(saved) << Json::stringify(pic);
    out("  Saved your Expert 1 settings for Undo.\n");
  }
  s.light = static_cast<int>(pic["backlight"].as_number(std::atof(pic["backlight"].as_string("28").c_str())));
  s.view_ct = Json::stringify(pic["colorTemperature"]);
  s.view_gamut = Json::stringify(pic["colorGamut"]);
  out("  Expert 1 active (light %d, colour temperature %s, gamut %s).\n", s.light, s.view_ct.c_str(), s.view_gamut.c_str());
  // 2. Meter and squares, before anything changes.
  if (!meter_up(tv)) return pal_fail(6, "The meter did not answer.");
  if (!squares_visible(tv)) return pal_fail(6, "The test squares are not reaching the meter (is the Pal session open on Bloob's Calibrate screen?).");
  // 3. The meter sees the middle window and only it.
  out("== Is the meter in the middle?\n");
  Reading c;
  if (!read_avg(tv, {940, 940, 940}, 1, &c)) return pal_fail(2, "The meter failed: " + tv.last_error());
  struct Side {
    const char *name;
    int x, y;
  };
  // Each one right beside the middle window: a meter near an edge sees part of it.
  const Side sides[] = {{"left", 1313 - 1214, 738}, {"right", 1313 + 1214, 738}, {"above", 1313, 738 - 683}, {"below", 1313, 738 + 683}};
  for (const Side &d : sides) {
    if (!tv.show_at({940, 940, 940}, d.x, d.y)) return pal_fail(2, "Could not draw the centring window: " + tv.last_error());
    Vec3 x;
    if (!tv.read_shown(&x)) return pal_fail(2, "The meter failed: " + tv.last_error());
    out("  window %s of the middle: %.2f cd/m2 (middle %.1f)\n", d.name, x.y, c.xyz.y);
    if (x.y > 0.05 * c.xyz.y) return pal_fail(6, std::string("The meter also sees a window drawn ") + d.name + " of the middle: hang it flat in the middle of the screen.");
  }
  out("  Yes: only the middle window reaches it.\n");
  // 4. Processing that moves readings, off and read back (put back if the start fails later).
  out("== Picture processing off\n");
  std::string processing = read_processing(tv);
  if (!exists(data_dir() + "/expert1-processing.json")) std::ofstream(data_dir() + "/expert1-processing.json") << processing;
  processing_off(tv, Json::parse(processing));
  Json now = Json::parse(read_processing(tv));
  for (const Processing &p : kProcessing)
    if (now[p.category].has(p.key) && now[p.category][p.key].as_string() != p.off)
      return restore_processing(tv), pal_fail(6, std::string("Picture processing still on after switching it off: ") + p.key);
  out("  Read back: all off.\n");
  // 5. Warm-up: white steady over three readings an interval apart (B4: panel and meter together).
  out("== Warming up: waiting for white to hold steady\n");
  int interval = getenv("BLOOB_PAL_WARMUP_INTERVAL_S") ? atoi(getenv("BLOOB_PAL_WARMUP_INTERVAL_S")) : 60;
  int most = getenv("BLOOB_PAL_WARMUP_MAX_S") ? atoi(getenv("BLOOB_PAL_WARMUP_MAX_S")) : 1800;
  std::vector<double> whites;
  auto w0 = std::chrono::steady_clock::now();
  for (;;) {
    Reading w;
    if (!read_avg(tv, {940, 940, 940}, 2, &w)) return restore_processing(tv), pal_fail(2, "The meter failed: " + tv.last_error());
    whites.push_back(w.xyz.y);
    if (whites.size() >= 3) {
      double lo = *std::min_element(whites.end() - 3, whites.end()), hi = *std::max_element(whites.end() - 3, whites.end());
      double spread = 100 * (hi - lo) / std::max(1e-6, (hi + lo) / 2);
      out("  white %.2f cd/m2; last three within %.2f%%\n", w.xyz.y, spread);
      if (spread <= 0.5) break;
    } else {
      out("  white %.2f cd/m2\n", w.xyz.y);
    }
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - w0).count() > most)
      return restore_processing(tv), pal_fail(6, "White did not hold steady within the warm-up time: the panel or the meter is still drifting.");
    for (int i = 0; i < interval && !g_stop; i++) sleep(1);
    if (g_stop) return restore_processing(tv), pal_fail(2, "Stopped.");
  }
  // 6. Calman's SDR start: LG's factory data, gamma switches off, BT.709 matrix neutral.
  //    From here on a failure undoes everything.
  out("== Starting from LG's factory data, as Calman does (gamma switches off, matrix neutral)\n");
  save_session(s);  // the viewing values are needed by every write from here
  auto undo_fail = [&](int rc, const std::string &why) {
    std::string note;
    undo_expert1(tv, &note);
    s.open = false;
    save_session(s);
    return pal_fail(rc, why + " Expert 1 is back to " + note + ".");
  };
  std::string err;
  if (!pal_write(tv, s, [&] { return tv.reset_factory() && tv.start_like_calman(); }, &err)) return undo_fail(2, "The TV refused Calman's start: " + err);
  Lut1D start;
  std::string start_raw;
  if (!read_1d(tv, &start, &s.start_fnv)) return undo_fail(2, "Could not read the 1D table: " + tv.last_error());
  {
    auto v = start.encode();
    std::string b = base64(v);
    write_file(pal_dir() + "/start-1d.json", "{\"fnv\":\"" + s.start_fnv + "\",\"data\":\"" + b + "\"}");
  }
  // 7. Which table entry does a level use? A flat step from 77% to 83% of the table: under the
  //    signal reading the 80% patch (code 765) darkens; under the code reading code 818 does.
  out("== Which table entry each level uses\n");
  Reading a0, b0, a1, b1;
  if (!read_avg(tv, {765, 765, 765}, 2, &a0) || !read_avg(tv, {818, 818, 818}, 2, &b0)) return undo_fail(2, "The meter failed: " + tv.last_error());
  if (!pal_write(tv, s, [&] { return tv.upload_1d(pal_plateau(start, 0.77, 0.83)); }, &err)) return undo_fail(2, "The index check's table was refused: " + err);
  bool measured = read_avg(tv, {765, 765, 765}, 2, &a1) && read_avg(tv, {818, 818, 818}, 2, &b1);
  if (!pal_write(tv, s, [&] { return tv.upload_1d(start); }, &err)) return undo_fail(2, "Could not put the start table back: " + err);
  std::string back_fnv;
  Lut1D back;
  if (!read_1d(tv, &back, &back_fnv) || back_fnv != s.start_fnv) return undo_fail(5, "The start table did not read back after the index check.");
  if (!measured) return undo_fail(2, "The meter failed: " + tv.last_error());
  double ra = a1.xyz.y / std::max(a0.xyz.y, 1e-6), rb = b1.xyz.y / std::max(b0.xyz.y, 1e-6);
  out("  80%% (code 765): %.3f of before; code 818: %.3f of before\n", ra, rb);
  if (ra < 0.97 && rb > 0.985) s.index = TableIndex::Signal;
  else if (rb < 0.97 && ra > 0.985) s.index = TableIndex::Code;
  else return undo_fail(6, "Could not tell which table entry a level uses (both or neither darkened).");
  out("  The table follows the %s.\n", s.index == TableIndex::Code ? "10-bit code (64 at entry 64, 940 at entry 940)" : "video signal (64 at entry 0, 940 at entry 1023)");
  // 8. Light, white first: what the corrected white will need, so no correction has to be
  //    followed by more light (which would brighten every level it just fixed).
  out("== Light: enough for a D65 white at %.0f nits\n", s.nits);
  if (!pal_dial_light(tv, s, s.nits, &err)) return undo_fail(2, err);
  {
    Reading w;
    Vec3 prim[3];
    const Patch ps[3] = {{940, 64, 64}, {64, 940, 64}, {64, 64, 940}};
    bool ok = read_avg(tv, {940, 940, 940}, 2, &w);
    for (int c = 0; c < 3 && ok; c++) ok = tv.measure(ps[c], &prim[c]);
    if (!ok) return undo_fail(2, "The meter failed: " + tv.last_error());
    double need = pal_white_need(w.xyz, prim, pal_sdr_target(s));
    s.headroom = std::max(1.0, need) * 1.01;
    out("  The corrected white needs %.1f%% more drive than now: white set to %.1f cd/m2 before correcting.\n", 100 * (need - 1), s.nits * s.headroom);
  }
  if (!pal_dial_light(tv, s, s.nits * s.headroom, &err)) return undo_fail(2, err);
  // 9. How much the meter wobbles: three readings of white and of 50% grey.
  out("== How steady the meter reads\n");
  Reading w, m;
  double sw = 0, sm = 0;
  if (!read_avg(tv, {940, 940, 940}, 3, &w, &sw) || !read_avg(tv, {502, 502, 502}, 3, &m, &sm)) return undo_fail(2, "The meter failed: " + tv.last_error());
  s.wobble_white = std::max(sw, 0.05);
  s.wobble_mid = std::max(sm, 0.05);
  out("  white within %.2f%%, 50%% within %.2f%%\n", sw, sm);
  s.open = true;
  save_session(s);
  tv.finish();
  out("== Ready. Next: grey-report.\n");
  return pal_result(0, "{\"ok\":true,\"kind\":\"start\",\"nits\":" + num(s.nits, 1) + ",\"light\":" + std::to_string(s.light) + ",\"index\":\"" +
                           (s.index == TableIndex::Code ? "code" : "signal") + "\",\"wobble\":{\"white_pct\":" + num(s.wobble_white, 3) +
                           ",\"mid_pct\":" + num(s.wobble_mid, 3) + "},\"start_table\":\"" + s.start_fnv + "\",\"viewing\":{\"colorTemperature\":" +
                           s.view_ct + ",\"colorGamut\":" + s.view_gamut + "},\"warmup_readings\":" + std::to_string(whites.size()) + "}");
}

static int cmd_grey_report() {
  PalSession s;
  if (!load_session(&s)) return pal_fail(6, "No Pal session is open: run pal-start first.");
  TvDevice tv(config("expert1"));
  report_progress(tv);
  if (!meter_up(tv)) return pal_fail(2, "The meter did not answer.");
  out("== Grey report (as you watch: outside calibration mode)\n");
  PalReport r;
  std::string err;
  if (!pal_report(tv, s, &r, &err)) return pal_fail(2, err);
  tv.finish();
  return pal_result(0, r.json);
}

static int cmd_pal_propose(const std::string &report_path, double damp) {
  PalSession s;
  if (!load_session(&s)) return pal_fail(6, "No Pal session is open: run pal-start first.");
  std::string path = report_path.empty() ? pal_dir() + "/latest-report.json" : report_path;
  std::vector<GreyLevel> levels;
  Vec3 prim[3];
  std::string fnv;
  if (!load_report(path, &levels, prim, &fnv)) return pal_fail(2, "Could not read the report " + path);
  TvDevice tv(config("expert1"));
  Lut1D table;
  std::string now_fnv;
  if (!read_1d(tv, &table, &now_fnv)) return pal_fail(2, "Could not read the 1D table: " + tv.last_error());
  if (now_fnv != fnv) return pal_fail(2, "The table in the TV is not the one this report measured: take a new grey-report.");
  std::vector<std::string> notes;
  PalAdjust a = pal_propose(levels, table, prim, pal_sdr_target(s), damp, s.index, &notes);
  Lut1D next = pal_apply(table, a, s.index);
  GateResult g = pal_gate(next, table);
  s.proposals++;
  save_session(s);
  std::string j = "{\"ok\":true,\"kind\":\"proposal\",\"n\":" + std::to_string(s.proposals) + ",\"for_table\":\"" + fnv + "\",\"damp\":" + num(damp, 2) + ",\"pct\":[";
  for (size_t i = 0; i < a.pct.size(); i++) j += (i ? "," : "") + std::to_string(a.pct[i]);
  j += "],\"r\":" + list(a.mult[0]) + ",\"g\":" + list(a.mult[1]) + ",\"b\":" + list(a.mult[2]) + ",\"gate\":{\"ok\":" + (g.ok ? "true" : "false") +
       ",\"why\":" + Json::quote(g.why) + ",\"max_step\":" + num(g.max_step, 4) + "},\"notes\":[";
  for (size_t i = 0; i < notes.size(); i++) j += (i ? "," : "") + Json::quote(notes[i]);
  j += "]}";
  write_file(pal_dir() + "/proposal-" + std::to_string(s.proposals) + ".json", j);
  write_file(pal_dir() + "/latest-proposal.json", j);
  for (auto &n : notes) out("  %s\n", n.c_str());
  out("  Proposal %d written (the gate %s).\n", s.proposals, g.ok ? "accepts it" : ("would refuse it: " + g.why).c_str());
  return pal_result(0, j);
}

static bool parse_adjust(const Json &j, PalAdjust *a, std::string *why) {
  a->pct.clear();
  for (size_t i = 0; i < j["pct"].size(); i++) a->pct.push_back(static_cast<int>(j["pct"][i].as_int(-1)));
  const char *names[3] = {"r", "g", "b"};
  for (int c = 0; c < 3; c++) {
    a->mult[c].clear();
    for (size_t i = 0; i < j[names[c]].size(); i++) a->mult[c].push_back(j[names[c]][i].as_number(std::nan("")));
  }
  return pal_adjust_valid(*a, why);
}

static int cmd_apply_1d(const std::string &arg) {
  PalSession s;
  if (!load_session(&s)) return pal_fail(6, "No Pal session is open: run pal-start first.");
  TvDevice tv(config("expert1"));
  report_progress(tv);
  Lut1D current;
  std::string cur_fnv;
  if (!read_1d(tv, &current, &cur_fnv)) return pal_fail(2, "Could not read the 1D table: " + tv.last_error());
  Lut1D next;
  std::string what;
  GateResult g;
  if (arg == "--start") {
    // Back to the table pal-start measured from (Pal's "clear": a pass made a level worse).
    bool ok = false;
    Json j = Json::parse(slurp_file(pal_dir() + "/start-1d.json"), &ok);
    auto v = ok ? u16_from_base64(j["data"].as_string()) : std::vector<uint16_t>();
    if (v.size() != 3 * Lut1D::kSize) return pal_fail(2, "The start table is missing.");
    next = Lut1D::decode(v);
    g = pal_gate(next, current, 1.0, false);  // a table this TV already had: range and rising only
    what = "the start table";
  } else {
    bool ok = false;
    Json j = Json::parse(slurp_file(arg), &ok);
    if (!ok) return pal_fail(4, "Could not read " + arg + " as JSON.");
    if (j.has("for_table") && j["for_table"].as_string() != cur_fnv)
      return pal_fail(4, "This adjustment was made for another table: the TV's table changed since its report.");
    PalAdjust a;
    std::string why;
    if (!parse_adjust(j, &a, &why)) return pal_fail(4, "The adjustment is not valid: " + why);
    next = pal_apply(current, a, s.index);
    g = pal_gate(next, current);
    what = arg;
  }
  out("== Applying %s\n", what.c_str());
  if (!g.ok) return pal_fail(4, "The safety gate refused the table: " + g.why);
  out("  The safety gate accepts it (largest change %.4f).\n", g.max_step);
  std::vector<uint16_t> sent = next.encode();
  std::string sent_fnv = hex64(fnv1a(sent)), err, got_fnv;
  bool loaded = false;
  for (int attempt = 0; attempt < 2 && !loaded; attempt++) {
    if (attempt) out("  The table read back differently; sending it once more.\n");
    if (!pal_write(tv, s, [&] { return tv.upload_1d(next); }, &err)) return pal_fail(2, "The TV refused the table: " + err);
    Lut1D got;
    loaded = read_1d(tv, &got, &got_fnv) && got_fnv == sent_fnv;
  }
  if (!loaded)
    return pal_fail(5, "The TV did not load the table (it read back as " + got_fnv + ", not " + sent_fnv + ").",
                    ",\"sent\":\"" + sent_fnv + "\",\"read_back\":\"" + got_fnv + "\"");
  s.applied++;
  save_session(s);
  write_file(pal_dir() + "/applied-" + std::to_string(s.applied) + ".json",
             "{\"from\":\"" + cur_fnv + "\",\"to\":\"" + sent_fnv + "\",\"source\":" + Json::quote(what) + ",\"data\":\"" + base64(sent) + "\"}");
  out("  Loaded: the table read back equals the one sent.\n");
  out("== After report\n");
  PalReport r;
  if (!meter_up(tv)) return pal_fail(2, "The meter did not answer.");
  if (!pal_report(tv, s, &r, &err)) return pal_fail(2, err, ",\"loaded\":true");
  tv.finish();
  // Never worse (§9): compared with the session's first report.
  std::vector<GreyLevel> first;
  Vec3 p0[3];
  std::string worse = "[";
  if (load_report(pal_dir() + "/report-1.json", &first, p0, nullptr))
    for (const GreyLevel &l : r.levels)
      for (const GreyLevel &f : first)
        if (f.pct == l.pct && l.scored && l.de > f.de + std::max(0.3, 0.1 * f.de)) worse += std::string(worse.size() > 1 ? "," : "") + std::to_string(l.pct);
  worse += "]";
  if (worse != "[]") out("  Worse than at the start at %s%%: clear back (apply-1d --start) and diagnose.\n", worse.c_str());
  return pal_result(0, "{\"ok\":true,\"kind\":\"applied\",\"n\":" + std::to_string(s.applied) + ",\"loaded\":true,\"from\":\"" + cur_fnv + "\",\"to\":\"" +
                           sent_fnv + "\",\"gate\":{\"max_step\":" + num(g.max_step, 4) + ",\"max_bend\":" + num(g.max_bend, 5) +
                           "},\"worse_than_start\":" + worse + ",\"after\":" + r.json + "}");
}

static int cmd_pal_light(double goal) {
  PalSession s;
  if (!load_session(&s)) return pal_fail(6, "No Pal session is open: run pal-start first.");
  TvDevice tv(config("expert1"));
  report_progress(tv);
  if (!meter_up(tv)) return pal_fail(2, "The meter did not answer.");
  out("== Light: white to %.1f cd/m2\n", goal);
  std::string err;
  if (!pal_dial_light(tv, s, goal, &err)) return pal_fail(2, err);
  Reading w;
  if (!read_avg(tv, {940, 940, 940}, 2, &w)) return pal_fail(2, "The meter failed: " + tv.last_error());
  save_session(s);
  tv.finish();
  return pal_result(0, "{\"ok\":true,\"kind\":\"light\",\"light\":" + std::to_string(s.light) + ",\"white_Y\":" + num(w.xyz.y, 3) + "}");
}

static int cmd_pal_end(const std::string &how) {
  PalSession s;
  if (!load_session(&s)) return pal_fail(6, "No Pal session is open.");
  std::string result;
  if (how == "undo") {
    TvDevice tv(config("expert1"));
    std::string note;
    bool ok = undo_expert1(tv, &note);
    out("== Undo: Expert 1 back to %s\n", note.c_str());
    result = std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"kind\":\"end\",\"kept\":false,\"undone\":" + (ok ? "true" : "false") + ",\"note\":" + Json::quote(note);
  } else {
    out("== Session ended: the calibration stays\n");
    result = "{\"ok\":true,\"kind\":\"end\",\"kept\":true";
  }
  s.open = false;
  save_session(s);
  // Everything from this session moves to history/ (nothing is deleted).
  std::string hist = data_dir() + "/history", dest = hist + "/pal-" + s.started;
  mkdir(hist.c_str(), 0755);
  if (g_log) {
    std::fclose(g_log);
    g_log = nullptr;
  }
  bool moved = ::rename(pal_dir().c_str(), dest.c_str()) == 0;
  out("  Session files: %s\n", moved ? dest.c_str() : "left in pal/ (could not move them)");
  return pal_result(0, result + ",\"history\":" + Json::quote(moved ? dest : pal_dir()) + "}");
}

// Read-only. Does LG's pipeline flicker after a pattern change, and does the pattern delay
// clear it (B1)? Each change is read at once and again after the delay.
static int cmd_flashcheck() {
  TvDevice tv(config("expert1"));
  report_progress(tv);
  if (!meter_up(tv)) return pal_fail(2, "The meter did not answer.");
  if (!squares_visible(tv)) return pal_fail(6, "The test squares are not reaching the meter.");
  tv.set_grey_between(false);
  int delay = tv.pattern_delay();
  out("== Flicker check: %d pattern changes, each read at once and after %.1f s\n", 6, delay / 1000.0);
  double worst_y = 0, worst_uv = 0;
  const int codes[2] = {502, 721};
  for (int i = 0; i < 6 && !g_stop; i++) {
    Patch p{codes[i % 2], codes[i % 2], codes[i % 2]};
    tv.set_pattern_delay(0);
    Vec3 early, late;
    if (!tv.measure(p, &early)) return pal_fail(2, "The meter failed: " + tv.last_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(getenv("BLOOB_CAL_FAST") ? 0 : delay));
    if (!tv.read_shown(&late)) return pal_fail(2, "The meter failed: " + tv.last_error());
    double dy = 100 * std::fabs(early.y / std::max(late.y, 1e-6) - 1), dl = duv(early, late);
    worst_y = std::max(worst_y, dy);
    worst_uv = std::max(worst_uv, dl);
    out("  change %d: read at once, %.2f%% off in brightness and %.4f off in colour (u'v') from the settled reading\n", i + 1, dy, dl);
  }
  tv.set_pattern_delay(delay);
  tv.finish();
  bool flicker = worst_y > 1.0 || worst_uv > 0.003;
  out("== %s\n", flicker ? "Readings taken at once are off: LG's pipeline is still settling then. The delay before every reading covers it."
                         : "No flicker reached the meter.");
  return pal_result(0, std::string("{\"ok\":true,\"kind\":\"flashcheck\",\"flicker_seen\":") + (flicker ? "true" : "false") + ",\"worst_brightness_pct\":" +
                           num(worst_y, 2) + ",\"worst_colour_uv\":" + num(worst_uv, 4) + ",\"delay_ms\":" + std::to_string(delay) + "}");
}

// One run at a time: the pid of the running bloob-cal, or 0.
static bool another_running(const std::string &pidfile) {
  std::ifstream f(pidfile);
  long pid = 0;
  f >> pid;
  if (pid <= 0 || pid == getpid() || kill(static_cast<pid_t>(pid), 0) != 0) return false;
  std::ifstream cmd("/proc/" + std::to_string(pid) + "/cmdline");
  std::string c((std::istreambuf_iterator<char>(cmd)), std::istreambuf_iterator<char>());
  return c.find("bloob-cal") != std::string::npos;
}

int main(int argc, char **argv) {
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
  signal(SIGHUP, on_signal);
  signal(SIGPIPE, SIG_IGN);
  setvbuf(stdout, nullptr, _IOLBF, 0);
  std::string cmd = argc > 1 ? argv[1] : "";
  if (geteuid() != 0 && !getenv("BLOOB_CAL_ANY_USER")) {
    out("Run bloob-cal on the TV as root.\n");
    return 1;
  }
  static const std::vector<std::string> kPal = {"pal-start", "grey-report", "pal-propose", "apply-1d", "pal-light", "pal-end", "flashcheck"};
  bool pal = std::find(kPal.begin(), kPal.end(), cmd) != kPal.end();
  if (cmd != "check" && cmd != "test" && cmd != "sdr" && cmd != "undo" && cmd != "patterns" && !pal) {
    out("usage: bloob-cal check | test | sdr [nits] [quick|thorough] | undo | patterns\n"
        "       bloob-cal pal-start [nits] | grey-report | pal-propose [report.json] [damp] | apply-1d <proposal.json>|--start\n"
        "                 | pal-light <white cd/m2> | pal-end ok|undo | flashcheck\n");
    return 1;
  }
  mkdir(data_dir().c_str(), 0755);
  std::string pidfile = data_dir() + "/bloob-cal.pid";
  if (another_running(pidfile)) {
    out("Another calibration run is still going; stop it first.\n");
    return 3;
  }
  { std::ofstream(pidfile) << getpid(); }
  // The run's own log, opened only now that this run holds the lock: a refused second
  // start must not wipe the log of the run that is going (seen on the TV).
  // Pal's commands answer on their own output (Pal reads it over SSH) and keep a session log.
  if (pal) {
    mkdir(pal_dir().c_str(), 0755);
    g_log = std::fopen((pal_dir() + "/session.log").c_str(), "a");
    if (g_log) std::fprintf(g_log, "\n### %s %s\n", stamp_now().c_str(), cmd.c_str());
  } else if (!isatty(1)) {
    int fd = ::open((data_dir() + "/last-run.log").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
      dup2(fd, 1);
      dup2(fd, 2);
      if (fd > 2) ::close(fd);
    }
  }
  g_status.path = env_or("BLOOB_CAL_STATUS", data_dir() + "/status.json");
  g_status.action = cmd;
  // Quick is the default for sdr: close to Calman's AutoCal, about a third of the readings.
  bool quick = !(argc > 3 && std::string(argv[3]) == "thorough");
  g_status.expected = cmd == "check" ? 17 : cmd == "test" ? 28 : cmd == "sdr" ? (quick ? 1950 : 4200) : cmd == "pal-start" ? 40 : cmd == "grey-report" ? 46
                     : cmd == "apply-1d" ? 46 : cmd == "pal-light" ? 8 : cmd == "flashcheck" ? 16 : 0;
  g_status.write();
  int rc = 0;
  if (cmd == "check") rc = cmd_check();
  else if (cmd == "patterns") rc = cmd_patterns();
  else if (cmd == "test") rc = cmd_test();
  else if (cmd == "sdr") rc = cmd_sdr(argc > 2 ? std::max(80.0, std::min(250.0, std::atof(argv[2]))) : 140.0, quick);
  else if (cmd == "pal-start") rc = cmd_pal_start(argc > 2 ? std::max(80.0, std::min(250.0, std::atof(argv[2]))) : 140.0);
  else if (cmd == "grey-report") rc = cmd_grey_report();
  else if (cmd == "pal-propose") rc = cmd_pal_propose(argc > 2 ? argv[2] : "", argc > 3 ? std::max(0.2, std::min(1.0, std::atof(argv[3]))) : 1.0);
  else if (cmd == "apply-1d") rc = argc > 2 ? cmd_apply_1d(argv[2]) : pal_fail(1, "apply-1d needs a proposal file or --start");
  else if (cmd == "pal-light") rc = argc > 2 ? cmd_pal_light(std::max(20.0, std::min(400.0, std::atof(argv[2])))) : pal_fail(1, "pal-light needs a white level in cd/m2");
  else if (cmd == "pal-end") rc = cmd_pal_end(argc > 2 ? argv[2] : "ok");
  else if (cmd == "flashcheck") rc = cmd_flashcheck();
  else rc = cmd_undo();
  g_status.state = g_stop ? "stopped" : rc == 0 ? "done" : "failed";
  if (cmd == "test") g_status.result = "{\"ok\":" + std::string(rc == 0 ? "true" : "false") + ",\"passed\":" + std::to_string(pass_count) +
                                       ",\"failed\":" + std::to_string(fail_count) + "}";
  g_status.write();
  { std::ofstream(pidfile) << 0; }
  if (g_log) std::fclose(g_log);
  return rc;
}
