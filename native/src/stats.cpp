#include "stats.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace stats {

static std::string read_file(const char *path) {
  std::ifstream in(path);
  if (!in) return "";
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

long long parse_net_rx(const std::string &text, std::string *busiest) {
  std::istringstream in(text);
  std::string line;
  long long total = 0, most = -1;
  bool any = false;
  while (std::getline(in, line)) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) continue;  // the two header lines
    std::string name = line.substr(0, colon);
    name.erase(0, name.find_first_not_of(' '));
    if (name.empty() || name == "lo") continue;
    long long rx = 0;
    if (std::sscanf(line.c_str() + colon + 1, "%lld", &rx) != 1) continue;
    total += rx;
    any = true;
    if (rx > most) {
      most = rx;
      if (busiest) *busiest = name;
    }
  }
  return any ? total : -1;
}

bool parse_cpu_total(const std::string &text, long long *total, long long *idle) {
  if (text.compare(0, 4, "cpu ") != 0) return false;
  std::istringstream in(text.substr(4, text.find('\n') - 4));
  long long v = 0, sum = 0, idle_ticks = 0;
  int i = 0;
  while (in >> v) {
    sum += v;
    if (i == 3 || i == 4) idle_ticks += v;  // idle and iowait
    i++;
  }
  if (i < 4) return false;
  *total = sum;
  *idle = idle_ticks;
  return true;
}

long long parse_self_ticks(const std::string &text) {
  // The command name (field 2) is in parentheses and may contain spaces.
  size_t close = text.rfind(')');
  if (close == std::string::npos) return -1;
  std::istringstream in(text.substr(close + 1));
  std::string field;
  long long utime = -1, stime = -1;
  // After ')': state is field 3; utime and stime are fields 14 and 15.
  for (int f = 3; f <= 15 && in >> field; f++) {
    if (f == 14) utime = std::atoll(field.c_str());
    if (f == 15) stime = std::atoll(field.c_str());
  }
  return utime >= 0 && stime >= 0 ? utime + stime : -1;
}

long long read_net_rx(std::string *busiest) { return parse_net_rx(read_file("/proc/net/dev"), busiest); }

bool read_cpu_total(long long *total, long long *idle) { return parse_cpu_total(read_file("/proc/stat"), total, idle); }

long long read_self_ticks() { return parse_self_ticks(read_file("/proc/self/stat")); }

void RateMeter::reset() {
  samples_.clear();
  first_at_ = -1;
  first_bytes_ = 0;
  peak_ = 0;
}

void RateMeter::add(long long now, long long bytes) {
  if (bytes < 0) return;
  if (first_at_ < 0) {
    first_at_ = now;
    first_bytes_ = bytes;
  }
  if (!samples_.empty() && bytes < samples_.back().bytes) {  // counter reset (interface came back)
    reset();
    first_at_ = now;
    first_bytes_ = bytes;
  }
  samples_.push_back({now, bytes});
  while (samples_.size() > 2 && now - samples_.front().at > 10000) samples_.pop_front();
  double m = mbps();
  if (m > peak_) peak_ = m;
}

double RateMeter::mbps(long long window_ms) const {
  if (samples_.size() < 2) return -1;
  const Sample &last = samples_.back();
  const Sample *from = &samples_.front();
  for (auto it = samples_.rbegin(); it != samples_.rend(); ++it) {
    from = &*it;
    if (last.at - it->at >= window_ms) break;
  }
  long long dt = last.at - from->at;
  if (dt <= 0) return -1;
  return static_cast<double>(last.bytes - from->bytes) * 8.0 / (static_cast<double>(dt) / 1000.0) / 1e6;
}

double RateMeter::average_mbps() const {
  if (samples_.empty() || samples_.back().at <= first_at_) return -1;
  return static_cast<double>(samples_.back().bytes - first_bytes_) * 8.0 /
         (static_cast<double>(samples_.back().at - first_at_) / 1000.0) / 1e6;
}

long long RateMeter::bytes() const { return samples_.empty() ? 0 : samples_.back().bytes - first_bytes_; }

std::string mbps_text(double mbps) {
  if (mbps < 0) return "\xE2\x80\x94";
  char buf[32];
  if (mbps < 1) std::snprintf(buf, sizeof buf, "%.0f kbps", mbps * 1000);
  else if (mbps < 100) std::snprintf(buf, sizeof buf, "%.1f Mbps", mbps);
  else std::snprintf(buf, sizeof buf, "%.0f Mbps", mbps);
  return buf;
}

std::string bytes_text(long long bytes) {
  char buf[32];
  if (bytes >= 1000000000LL) std::snprintf(buf, sizeof buf, "%.1f GB", static_cast<double>(bytes) / 1e9);
  else std::snprintf(buf, sizeof buf, "%.0f MB", static_cast<double>(bytes) / 1e6);
  return buf;
}

}  // namespace stats
