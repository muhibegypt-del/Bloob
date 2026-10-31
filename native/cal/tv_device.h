#pragma once
// The real TV as a cal::Device (CALIBRATION-PLAN.md §2, §9).
// - Calibration and pattern commands go to luna://com.webos.service.pqcontroller,
//   which answers luna-send from root on the owner's G2 (C0.3). Payloads too large
//   for one luna-send argument (the 3D LUT) go over the second-screen socket (ssap.h).
// - Only the commands and picture modes on the allow-list can be sent.
// - Leaving (normally, on error or on Ctrl-C) always ends calibration mode and turns
//   the patterns off.
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "device.h"
#include "json.h"
#include "meter.h"
#include "ssap.h"

namespace cal {

enum class Family { Sdr, Hdr10, DolbyVision };

struct TvConfig {
  MeterConfig meter;
  std::string manifest;   // the probe app's signed manifest (read at run time, never committed)
  std::string key_file;   // Bloob's own client key (mode 600)
  std::string picture_mode = "expert1";
  Family family = Family::Sdr;
  bool grey_between = true;  // full-screen 18% grey after each reading (Calman's OLED default)
  // How long a new patch settles before the meter reads it. LG's pipeline flickers pink or
  // green for a moment after a pattern change; 1.5–2 s is the calibrators' figure
  // (research/LG-calibration-best-practices.md B1). BLOOB_PATTERN_DELAY_MS overrides.
  int pattern_delay_ms = 2000;
  bool verbose = false;
};

// Set by the Ctrl-C handler; every TV and meter step checks it.
extern volatile int g_stop;

class TvDevice : public Device {
 public:
  explicit TvDevice(const TvConfig &c);
  ~TvDevice() override { finish(); }

  bool open_meter(std::string *error) { return meter_.open(cfg_.meter, error); }
  bool meter_session() const { return meter_.session(); }

  bool measure(const Patch &p, Vec3 *XYZ) override;
  // A reading of whatever is on screen now, without drawing anything.
  bool read_shown(Vec3 *XYZ);
  bool upload_1d(const Lut1D &lut) override;
  bool upload_3d(Slot slot, const Lut3D &lut) override;
  bool reset_factory() override;
  bool set_light(int value) override;
  int light() const override { return light_; }
  bool set_tonemap(const int params[7]) override;
  bool set_calibration_mode(bool on) override;
  // Calman's SDR start, inside calibration mode (tv_device.cpp): the bypass part (gamma
  // switches off, BT.709 identity), then contrast 85, brightness 50, colour 50.
  bool calman_bypass();
  bool start_like_calman();
  bool load_dolby_config(const std::string &) override { return false; }  // no USB stick yet

  // Read-only: a getter (GET_1D_DPG_DATA, GET_3D_LUT_DATA, GET_3BY3_GAMUT_DATA…).
  bool get_table(const std::string &command, Json *reply);
  bool patterns_off();
  // One box in the middle (the 10% window), over a full-screen black background or on
  // its own over whatever is on screen (bscpylgtv's "overlay"). For bloob-cal patterns.
  bool show_box(const Patch &p, bool background);
  // The measuring window moved to (x, y) (panel pixels, top left), on black: the
  // meter-centring check draws it beside the meter and expects darkness.
  bool show_at(const Patch &p, int x, int y);
  void set_pattern_delay(int ms) { cfg_.pattern_delay_ms = ms; }
  int pattern_delay() const { return cfg_.pattern_delay_ms; }
  void set_grey_between(bool on) { cfg_.grey_between = on; }
  // Picture settings through the settings service.
  bool read_picture(Json *settings);
  bool set_picture_mode(const std::string &mode);
  // Several picture settings at once, as a JSON object (Undo puts the saved ones back).
  bool set_picture_settings(const std::string &settings_json) { return set_settings("picture", settings_json); }
  // Any settings category. Keys this TV does not have are left out of the reply
  // (asked one by one if the TV refuses the whole list); false if none was read.
  bool read_settings(const std::string &category, const std::vector<std::string> &keys, Json *settings);
  bool set_settings(const std::string &category, const std::string &settings_json);
  void set_light_cache(int v) { light_ = v; }
  // Ends calibration mode and turns patterns off (idempotent).
  void finish();
  const std::string &last_error() const { return error_; }
  long readings() const { return readings_; }
  // Called after every meter reading (progress for the status file).
  std::function<void(long)> on_reading;
  // Called when a new patch is shown (the screen says when a dark one will read slowly).
  std::function<void(const Patch &)> on_patch;

 private:
  bool pq(const std::string &command, const std::string &fields, bool write = true);
  bool luna(const std::string &uri, const std::string &payload, Json *reply);
  bool show(const Patch &p);

  TvConfig cfg_;
  Meter meter_;
  Ssap ssap_;
  bool in_session_ = false;
  bool patterns_on_ = false;
  bool showing_ = false;
  Patch shown_;
  long readings_ = 0;
  int light_ = -1;
  std::string error_;
};

// The base64 the TV's `data` field carries, for any little-endian payload.
std::string base64_bytes(const unsigned char *data, size_t n);

}  // namespace cal
