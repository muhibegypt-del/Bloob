#pragma once
// Runtime inspector (R5d). A read-only snapshot of Bloob's state, written as
// JSON at most once a second, atomically, to /tmp/bloob-state.json (or
// $BLOOB_INSPECT_PATH). native/scripts/tv-inspect.sh reads it over SSH.
//
// Memory is reported per ownership domain, each with what it measures (study
// C9, O3): process memory from /proc, the GPU estimate by class, decoded pixels
// on the CPU, the service's own Node memory. Never URLs, keys or release names.
#include <string>

// A tiny JSON writer: enough for the snapshot, escapes strings.
class JsonOut {
 public:
  JsonOut &begin() { return raw("{"), first_ = true, *this; }
  JsonOut &end() { return raw("}"), first_ = false, *this; }
  JsonOut &key(const char *k);
  JsonOut &obj(const char *k) { key(k); return raw("{"), first_ = true, *this; }
  JsonOut &arr(const char *k) { key(k); return raw("["), first_ = true, *this; }
  JsonOut &end_arr() { return raw("]"), first_ = false, *this; }
  JsonOut &item_str(const std::string &v);
  JsonOut &num(const char *k, long long v);
  JsonOut &dbl(const char *k, double v);
  JsonOut &str(const char *k, const std::string &v);
  JsonOut &boolean(const char *k, bool v);
  const std::string &text() const { return s_; }

 private:
  JsonOut &raw(const char *t) {
    s_ += t;
    return *this;
  }
  void comma();
  std::string s_;
  bool first_ = true;
};

struct ProcMemory {
  long long rss_kb = -1, hwm_kb = -1, swap_kb = -1, threads = -1;
};
// This process, from /proc/self/status (VmRSS, VmHWM, VmSwap, Threads).
ProcMemory read_proc_memory();

class Inspector {
 public:
  void init();
  bool due(long long now) const { return enabled_ && now - last_write_ >= 1000; }
  // Writes `json` if it differs from the last snapshot. Disables itself on failure.
  void write(const std::string &json, long long now);
  const std::string &path() const { return path_; }

 private:
  std::string path_, last_;
  long long last_write_ = -1000000;
  bool enabled_ = true;
};
