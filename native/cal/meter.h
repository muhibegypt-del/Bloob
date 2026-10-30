#pragma once
// The Spyder5 through Argyll's spotread (CALIBRATION-PLAN.md §4.2). Argyll is AGPL,
// so Bloob runs the copy the owner installed with the Spyder probe app; nothing of it
// is bundled. One session is kept open in a pseudo-terminal (spotread reads its keys
// from a terminal), so the meter starts once; if that fails, one spotread per reading.
#include <string>
#include <sys/types.h>

#include "colour.h"

namespace cal {

struct MeterConfig {
  std::string spotread;            // path to spotread
  std::string libs;                // its LD_LIBRARY_PATH
  std::string loader;              // e.g. /lib/ld-linux.so.3 (the probe runs it this way); empty: exec directly
  std::string ccss;                // spectral correction, -X (empty: none)
  std::string display;             // -y: "c" (refresh: OLED, as the correction file says) or "l"; empty: spotread's default
  bool no_init_cal = false;        // -N: the Spyder's start-up calibration was done earlier this boot
  std::string refresh_hz;          // -Y R:<Hz>: the screen's refresh, so spotread does not measure it
  std::string extra;               // extra options, e.g. "-Y r"
};

class Meter {
 public:
  ~Meter() { close(); }
  bool open(const MeterConfig &c, std::string *error);  // a session, or the one-shot fallback
  bool read(Vec3 *XYZ, std::string *error);
  void close();
  bool session() const { return pid_ > 0; }

 private:
  bool read_one_shot(Vec3 *XYZ, std::string *error);
  bool read_session(Vec3 *XYZ, std::string *error);
  bool wait_for(const std::string &marker, int timeout_ms, std::string *seen);
  MeterConfig cfg_;
  int fd_ = -1;
  pid_t pid_ = -1;
  std::string buf_;
  bool fallback_ = false;
};

// "Result is XYZ: X Y Z, ..." → XYZ. False if the line is not there.
bool parse_spotread(const std::string &text, Vec3 *XYZ);

}  // namespace cal
