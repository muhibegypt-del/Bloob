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
  bool upload_1d(const Lut1D &lut) override;
  bool upload_3d(Slot slot, const Lut3D &lut) override;
  bool reset_factory() override;
  bool set_light(int value) override;
  int light() const override { return light_; }
  bool set_tonemap(const int params[7]) override;
  bool set_calibration_mode(bool on) override;
  bool load_dolby_config(const std::string &) override { return false; }  // no USB stick yet

  // Read-only: a getter (GET_1D_DPG_DATA, GET_3D_LUT_DATA, GET_3BY3_GAMUT_DATA…).
  bool get_table(const std::string &command, Json *reply);
  bool patterns_off();
  // Picture settings through the settings service.
  bool read_picture(Json *settings);
  bool set_picture_mode(const std::string &mode);
  // Several picture settings at once, as a JSON object (Undo puts the saved ones back).
  bool set_picture_settings(const std::string &settings_json);
  void set_light_cache(int v) { light_ = v; }
  // Ends calibration mode and turns patterns off (idempotent).
  void finish();
  const std::string &last_error() const { return error_; }
  long readings() const { return readings_; }
  // Called after every meter reading (progress for the status file).
  std::function<void(long)> on_reading;

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
