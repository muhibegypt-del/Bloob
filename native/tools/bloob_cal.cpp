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

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include "colour.h"
#include "lut.h"
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
static void out(const char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  std::fputs(buf, stdout);
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
  if (cmd != "check" && cmd != "test" && cmd != "sdr" && cmd != "undo" && cmd != "patterns") {
    out("usage: bloob-cal check | test | sdr [nits] [quick|thorough] | undo | patterns\n");
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
  if (!isatty(1)) {
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
  g_status.expected = cmd == "check" ? 17 : cmd == "test" ? 28 : cmd == "sdr" ? (quick ? 1950 : 4200) : 0;
  g_status.write();
  int rc = 0;
  if (cmd == "check") rc = cmd_check();
  else if (cmd == "patterns") rc = cmd_patterns();
  else if (cmd == "test") rc = cmd_test();
  else if (cmd == "sdr") rc = cmd_sdr(argc > 2 ? std::max(80.0, std::min(250.0, std::atof(argv[2]))) : 140.0, quick);
  else rc = cmd_undo();
  g_status.state = g_stop ? "stopped" : rc == 0 ? "done" : "failed";
  if (cmd == "test") g_status.result = "{\"ok\":" + std::string(rc == 0 ? "true" : "false") + ",\"passed\":" + std::to_string(pass_count) +
                                       ",\"failed\":" + std::to_string(fail_count) + "}";
  g_status.write();
  { std::ofstream(pidfile) << 0; }
  return rc;
}
