#include "tv_device.h"

#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace cal {

volatile int g_stop = 0;

static const char *kPq = "luna://com.webos.service.pqcontroller/";

// The allow-list (§9.1): nothing else ever reaches the TV.
static const std::set<std::string> kWrites = {"CAL_START", "CAL_END", "BACKLIGHT_UI_DATA", "CONTRAST_UI_DATA", "BRIGHTNESS_UI_DATA",
                                              "COLOR_UI_DATA", "1D_DPG_DATA", "BT709_3D_LUT_DATA", "BT2020_3D_LUT_DATA",
                                              "BT709_3BY3_GAMUT_DATA", "BT2020_3BY3_GAMUT_DATA", "1D_2_2_EN", "1D_0_45_EN",
                                              "3BY3_GAMUT_EN", "1D_TONEMAP_PARAM", "PTN_SINGLE_WINBOX_ATTR", "PTN_CTRL"};
static const std::set<std::string> kReads = {"GET_1D_DPG_DATA", "GET_3D_LUT_DATA", "GET_3BY3_GAMUT_DATA", "GET_1D_2_2_EN", "GET_1D_0_45_EN"};
static const std::set<std::string> kModes = {"expert1", "expert2", "hdr_cinema", "dolby_cinema_dark"};

std::string base64_bytes(const unsigned char *d, size_t n) {
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string o;
  o.reserve((n + 2) / 3 * 4);
  for (size_t i = 0; i < n; i += 3) {
    unsigned v = d[i] << 16 | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
    o += t[v >> 18 & 63];
    o += t[v >> 12 & 63];
    o += i + 1 < n ? t[v >> 6 & 63] : '=';
    o += i + 2 < n ? t[v & 63] : '=';
  }
  return o;
}

static std::string u16_data(const std::vector<uint16_t> &v, int opt = 1) {
  std::vector<unsigned char> b;
  for (uint16_t x : v) {
    b.push_back(static_cast<unsigned char>(x & 0xff));
    b.push_back(static_cast<unsigned char>(x >> 8));
  }
  return ",\"data\":\"" + base64_bytes(b.data(), b.size()) + "\",\"dataCount\":" + std::to_string(v.size()) +
         ",\"dataType\":\"unsigned integer16\",\"dataOpt\":" + std::to_string(opt);
}

static std::string reset_data(const char *type) { return std::string(",\"data\":\"\",\"dataCount\":0,\"dataType\":\"") + type + "\",\"dataOpt\":2"; }

TvDevice::TvDevice(const TvConfig &c) : cfg_(c) {}

// luna-send as a child process (no shell, so nothing in the payload is interpreted).
bool TvDevice::luna(const std::string &uri, const std::string &payload, Json *reply) {
  int out[2];
  if (pipe(out) != 0) {
    error_ = "pipe failed";
    return false;
  }
  pid_t pid = fork();
  if (pid == 0) {
    dup2(out[1], 1);
    dup2(out[1], 2);
    ::close(out[0]);
    const char *bin = getenv("BLOOB_LUNA_SEND");
    execlp(bin ? bin : "luna-send", "luna-send", "-n", "1", uri.c_str(), payload.c_str(), static_cast<char *>(nullptr));
    _exit(127);
  }
  ::close(out[1]);
  std::string text;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  for (;;) {
    int left = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
    pollfd pf{out[0], POLLIN, 0};
    if (left <= 0 || poll(&pf, 1, left) <= 0) break;
    char tmp[8192];
    ssize_t r = ::read(out[0], tmp, sizeof tmp);
    if (r <= 0) break;
    text.append(tmp, static_cast<size_t>(r));
  }
  ::close(out[0]);
  kill(pid, SIGTERM);
  waitpid(pid, nullptr, 0);
  size_t brace = text.find('{');
  bool ok = false;
  *reply = brace == std::string::npos ? Json() : Json::parse(text.substr(brace, text.rfind('}') - brace + 1), &ok);
  if (!ok) {
    error_ = "no JSON reply from " + uri + ": " + text.substr(0, 200);
    return false;
  }
  return true;
}

bool TvDevice::pq(const std::string &command, const std::string &fields, bool write) {
  if (g_stop && command != "CAL_END" && command != "PTN_CTRL") {
    error_ = "stopped";
    return false;
  }
  if (write ? !kWrites.count(command) : !kReads.count(command)) {
    error_ = "refused by Bloob's allow-list: " + command;
    return false;
  }
  std::string payload = "{\"command\":\"" + command + "\"" + (write && command.rfind("PTN_", 0) != 0 ? ",\"profileNo\":0" : "") + ",\"programID\":1" + fields + "}";
  if (!write) payload = "{\"command\":\"" + command + "\"}";
  Json reply;
  bool sent;
  // Patterns go over the second-screen socket, as the Spyder probe drew them: on this TV a
  // pattern sent by luna-send did not appear (29 October 2026), most likely because it
  // lasts only while the client that set it is connected. Large tables go there too.
  bool socket = payload.size() >= 100000 || (command.rfind("PTN_", 0) == 0 && !getenv("BLOOB_PATTERNS_LUNA"));
  auto t0 = std::chrono::steady_clock::now();
  if (!socket) {
    sent = luna(std::string(kPq) + (write ? "setExternalPqData" : "getExternalPqData"), payload, &reply);
  } else {
    // The second-screen socket (the TV asks once to allow Bloob).
    std::string uri = std::string("ssap://externalpq/") + (write ? "setExternalPqData" : "getExternalPqData");
    if (!ssap_.connected() && !ssap_.connect(cfg_.manifest, cfg_.key_file, &error_)) {
      error_ = command + " over the second-screen socket: " + error_;
      return false;
    }
    sent = ssap_.request(uri, payload, &reply, &error_);
    if (!sent) {
      // The TV's own error (on the owner's TV, "500 Application error"): once more on a
      // fresh connection, in case the session went bad, then say which command and route.
      std::string first = error_;
      ssap_.close();
      sent = ssap_.connect(cfg_.manifest, cfg_.key_file, &error_) && ssap_.request(uri, payload, &reply, &error_);
      if (!sent) error_ = command + " over the second-screen socket: " + first + (first == error_ ? "" : "; again: " + error_);
    }
  }
  if (!sent) return false;
  if (!reply["returnValue"].as_bool()) {
    // Turning patterns off while already off answers error 20 (as the probe found).
    if (command == "PTN_CTRL" && reply["errorCode"].as_int() == 20) return true;
    error_ = command + " refused: " + Json::stringify(reply).substr(0, 300);
    return false;
  }
  if (cfg_.verbose)
    std::printf("    [tv] %s ok (%s, %lld ms)\n", command.c_str(), socket ? "socket" : "luna-send",
                static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count()));
  return true;
}

bool TvDevice::get_table(const std::string &command, Json *reply) {
  if (!kReads.count(command)) {
    error_ = "refused by Bloob's allow-list: " + command;
    return false;
  }
  return luna(std::string(kPq) + "getExternalPqData", "{\"command\":\"" + command + "\"}", reply);
}

// The 10% window on a black background (Calman's OLED window), in the mode's range.
bool TvDevice::show(const Patch &p) { return show_box(p, true); }

bool TvDevice::show_box(const Patch &p, bool background) {
  static const int W = 1214, H = 683, X = 1313, Y = 738;
  int black = cfg_.family == Family::DolbyVision ? 0 : 64;
  auto box = [&](int id, int r, int g, int b, int w, int h, int x, int y) {
    return pq("PTN_SINGLE_WINBOX_ATTR", ",\"fillR\":" + std::to_string(r) + ",\"fillG\":" + std::to_string(g) + ",\"fillB\":" + std::to_string(b) +
                                            ",\"winId\":" + std::to_string(id) + ",\"width\":" + std::to_string(w) + ",\"height\":" +
                                            std::to_string(h) + ",\"startX\":" + std::to_string(x) + ",\"startY\":" + std::to_string(y));
  };
  if (background ? !box(0, black, black, black, 3840, 2160, 0, 0) || !box(1, p.r, p.g, p.b, W, H, X, Y) : !box(0, p.r, p.g, p.b, W, H, X, Y)) return false;
  if (!pq("PTN_CTRL", std::string(",\"enable\":\"true\",\"numOfBox\":") + (background ? "2" : "1") + ",\"ptnType\":0")) return false;
  patterns_on_ = true;
  return true;
}

bool TvDevice::patterns_off() {
  patterns_on_ = false;
  showing_ = false;
  return pq("PTN_CTRL", ",\"enable\":\"false\",\"numOfBox\":0,\"ptnType\":0");
}

bool TvDevice::measure(const Patch &p, Vec3 *XYZ) {
  if (g_stop) return false;
  // A repeat reading of the square already on screen needs no redraw. A new square
  // first gets a second of full-screen 18% grey (Calman's OLED pattern insertion),
  // which keeps the panel's average level, and its power limiter, steady.
  bool same = showing_ && p.r == shown_.r && p.g == shown_.g && p.b == shown_.b;
  if (!same) {
    if (showing_ && cfg_.grey_between) {
      int g = cfg_.family == Family::DolbyVision ? 184 : 64 + 158;
      if (pq("PTN_SINGLE_WINBOX_ATTR", ",\"fillR\":" + std::to_string(g) + ",\"fillG\":" + std::to_string(g) + ",\"fillB\":" + std::to_string(g) +
                                           ",\"winId\":0,\"width\":3840,\"height\":2160,\"startX\":0,\"startY\":0") &&
          pq("PTN_CTRL", ",\"enable\":\"true\",\"numOfBox\":1,\"ptnType\":0"))
        std::this_thread::sleep_for(std::chrono::milliseconds(getenv("BLOOB_CAL_FAST") ? 0 : 800));
    }
    if (!show(p)) {
      showing_ = false;
      return false;
    }
    shown_ = p;
    showing_ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(getenv("BLOOB_CAL_FAST") ? 0 : 400));  // desktop tests only
  }
  std::string err;
  bool ok = meter_.read(XYZ, &err);
  if (!ok) error_ = err;
  if (ok) {
    readings_++;
    if (on_reading) on_reading(readings_);
  }
  return ok && !g_stop;
}

bool TvDevice::upload_1d(const Lut1D &lut) { return pq("1D_DPG_DATA", u16_data(lut.encode())); }

bool TvDevice::upload_3d(Slot slot, const Lut3D &lut) {
  if (lut.n != 33) {
    error_ = "this TV takes 33-point 3D LUTs";
    return false;
  }
  return pq(slot == Slot::Bt709 ? "BT709_3D_LUT_DATA" : "BT2020_3D_LUT_DATA", u16_data(lut.encode()));
}

// LG's factory data for the mode in session (bscpylgtv's reset_factory_data_*).
bool TvDevice::reset_factory() {
  if (cfg_.family == Family::DolbyVision) return pq("1D_DPG_DATA", reset_data("unsigned integer16"));
  return pq("1D_2_2_EN", reset_data("unsigned integer16")) && pq("1D_0_45_EN", reset_data("unsigned integer16")) &&
         pq("BT709_3BY3_GAMUT_DATA", reset_data("float")) && pq("BT2020_3BY3_GAMUT_DATA", reset_data("float")) &&
         pq("BT709_3D_LUT_DATA", reset_data("unsigned integer16")) && pq("BT2020_3D_LUT_DATA", reset_data("unsigned integer16")) &&
         pq("1D_DPG_DATA", reset_data("unsigned integer16"));
}

bool TvDevice::set_light(int value) {
  if (value < 0 || value > 100) return false;
  if (!pq("BACKLIGHT_UI_DATA", u16_data({static_cast<uint16_t>(value)}))) return false;
  light_ = value;
  return true;
}

bool TvDevice::set_tonemap(const int params[7]) {
  if (cfg_.family != Family::Hdr10) return false;
  std::vector<uint16_t> v(params, params + 7);
  return pq("1D_TONEMAP_PARAM", u16_data(v));
}

bool TvDevice::set_calibration_mode(bool on) {
  if (on) {
    if (in_session_) return true;
    if (!kModes.count(cfg_.picture_mode)) {
      error_ = "refused by Bloob's allow-list: picture mode " + cfg_.picture_mode;
      return false;
    }
    if (!pq("CAL_START", ",\"picMode\":\"" + cfg_.picture_mode + "\"")) return false;
    in_session_ = true;
    return true;
  }
  if (!in_session_) return true;
  in_session_ = false;
  return pq("CAL_END", "");
}

bool TvDevice::read_picture(Json *settings) {
  Json r;
  if (!luna("luna://com.webos.settingsservice/getSystemSettings",
            "{\"category\":\"picture\",\"keys\":[\"pictureMode\",\"backlight\",\"contrast\",\"brightness\",\"color\",\"gamma\",\"colorGamut\",\"colorTemperature\"]}",
            &r))
    return false;
  *settings = r["settings"];
  return r["returnValue"].as_bool();
}

bool TvDevice::set_picture_mode(const std::string &mode) {
  if (!kModes.count(mode) && mode != "expert1") {
    error_ = "refused: picture mode " + mode;
    return false;
  }
  Json r;
  if (!luna("luna://com.webos.settingsservice/setSystemSettings", "{\"category\":\"picture\",\"settings\":{\"pictureMode\":\"" + mode + "\"}}", &r))
    return false;
  if (!r["returnValue"].as_bool()) {
    error_ = "picture mode refused: " + Json::stringify(r).substr(0, 200);
    return false;
  }
  return true;
}

bool TvDevice::read_settings(const std::string &category, const std::vector<std::string> &keys, Json *settings) {
  auto ask = [&](const std::vector<std::string> &ks, Json *r) {
    std::string list;
    for (const std::string &k : ks) list += (list.empty() ? "" : ",") + Json::quote(k);
    return luna("luna://com.webos.settingsservice/getSystemSettings", "{\"category\":" + Json::quote(category) + ",\"keys\":[" + list + "]}", r) &&
           (*r)["returnValue"].as_bool();
  };
  Json r;
  if (ask(keys, &r)) {
    *settings = r["settings"];
    for (const std::string &k : keys)
      if (settings->has(k)) return true;
    return false;
  }
  std::string j = "{";
  bool any = false;
  for (const std::string &k : keys) {
    Json one;
    if (!ask({k}, &one) || !one["settings"].has(k)) continue;
    j += (any ? "," : "") + Json::quote(k) + ":" + Json::stringify(one["settings"][k]);
    any = true;
  }
  *settings = Json::parse(j + "}");
  return any;
}

bool TvDevice::set_settings(const std::string &category, const std::string &settings_json) {
  Json r;
  if (!luna("luna://com.webos.settingsservice/setSystemSettings", "{\"category\":" + Json::quote(category) + ",\"settings\":" + settings_json + "}", &r))
    return false;
  if (!r["returnValue"].as_bool()) {
    error_ = "settings refused: " + Json::stringify(r).substr(0, 200);
    return false;
  }
  return true;
}

void TvDevice::finish() {
  int stop = g_stop;
  g_stop = 0;  // the closing commands must go through
  if (in_session_) set_calibration_mode(false);
  if (patterns_on_) patterns_off();
  g_stop = stop;
}

}  // namespace cal
