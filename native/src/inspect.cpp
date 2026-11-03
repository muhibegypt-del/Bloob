#include "inspect.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "json.h"
#include "log.h"

void JsonOut::comma() {
  if (!first_) s_ += ",";
  first_ = false;
}

JsonOut &JsonOut::key(const char *k) {
  comma();
  s_ += Json::quote(k);
  s_ += ":";
  return *this;
}

JsonOut &JsonOut::item_str(const std::string &v) {
  comma();
  s_ += Json::quote(v);
  return *this;
}

JsonOut &JsonOut::num(const char *k, long long v) {
  key(k);
  s_ += std::to_string(v);
  first_ = false;
  return *this;
}

JsonOut &JsonOut::dbl(const char *k, double v) {
  key(k);
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.2f", v);
  s_ += buf;
  first_ = false;
  return *this;
}

JsonOut &JsonOut::str(const char *k, const std::string &v) {
  key(k);
  s_ += Json::quote(v);
  first_ = false;
  return *this;
}

JsonOut &JsonOut::boolean(const char *k, bool v) {
  key(k);
  s_ += v ? "true" : "false";
  first_ = false;
  return *this;
}

ProcMemory read_proc_memory() {
  ProcMemory m;
  FILE *f = std::fopen("/proc/self/status", "r");
  if (!f) return m;
  char line[256];
  while (std::fgets(line, sizeof line, f)) {
    long long v = 0;
    if (std::sscanf(line, "VmRSS: %lld", &v) == 1) m.rss_kb = v;
    else if (std::sscanf(line, "VmHWM: %lld", &v) == 1) m.hwm_kb = v;
    else if (std::sscanf(line, "VmSwap: %lld", &v) == 1) m.swap_kb = v;
    else if (std::sscanf(line, "Threads: %lld", &v) == 1) m.threads = v;
  }
  std::fclose(f);
  return m;
}

void Inspector::init() {
  const char *p = std::getenv("BLOOB_INSPECT_PATH");
  path_ = p && *p ? p : "/tmp/bloob-state.json";
  logf("inspect", "state snapshots at %s", path_.c_str());
}

void Inspector::write(const std::string &json, long long now) {
  last_write_ = now;
  if (json == last_) return;
  std::string tmp = path_ + ".tmp";
  FILE *f = std::fopen(tmp.c_str(), "w");
  bool ok = f && std::fwrite(json.data(), 1, json.size(), f) == json.size();
  if (f) ok = std::fclose(f) == 0 && ok;
  if (ok) ok = std::rename(tmp.c_str(), path_.c_str()) == 0;
  if (!ok) {
    logf("inspect", "cannot write %s (%s); inspector off for this session", path_.c_str(), std::strerror(errno));
    enabled_ = false;
    return;
  }
  last_ = json;
}
