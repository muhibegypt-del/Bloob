#pragma once
// The TV's second-screen socket (ws://127.0.0.1:3000), the route bscpylgtv and the
// Spyder probe use for externalpq (CALIBRATION-PLAN.md §2.1). Bloob needs it only
// for payloads too large for a luna-send argument (the 3D LUT, ~290 KB of base64).
// Registers with LG's signed test manifest, read at run time from the probe app
// (never committed); the TV asks the owner to allow it once, and the key is kept
// with mode 600 in Bloob's data folder, never printed.
#include <string>

#include "json.h"

namespace cal {

class Ssap {
 public:
  ~Ssap() { close(); }
  // `manifest_path`: the signed manifest JSON; `key_path`: where the client key lives.
  bool connect(const std::string &manifest_path, const std::string &key_path, std::string *error);
  // One request: `uri` like "ssap://externalpq/setExternalPqData", `payload` JSON text.
  bool request(const std::string &uri, const std::string &payload, Json *reply, std::string *error, int timeout_ms = 20000);
  bool connected() const { return fd_ >= 0; }
  void close();

 private:
  bool send_text(const std::string &text);
  bool receive_text(std::string *text, int timeout_ms);
  bool read_exact(void *buf, size_t n, int timeout_ms);
  int fd_ = -1;
  int next_id_ = 1;
};

}  // namespace cal
