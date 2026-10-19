#pragma once
// Playback statistics for the player's info panel (Info, I or the blue key):
// what the TV's own counters say, sampled while the panel is open. Parsing is
// pure and unit-tested; reading /proc is a thin wrapper.
//
// What can be measured on this TV, and what cannot:
//  - Network: uMediaServer downloads the stream itself, so Bloob cannot count
//    the stream's bytes. It reads the TV's network counters (/proc/net/dev),
//    which during playback are almost all the stream.
//  - File bitrate: the average over the whole file, from its size and running
//    time. The live video bitrate is not reported by the TV's media pipeline.
//  - Buffer: whatever uMediaServer's bufferRange event reports.
#include <deque>
#include <string>

namespace stats {

// Received bytes summed over every interface except loopback, from the text
// of /proc/net/dev. `busiest` (optional) gets the interface with most bytes.
long long parse_net_rx(const std::string &proc_net_dev, std::string *busiest = nullptr);
// Total and idle CPU ticks from the first ("cpu") line of /proc/stat.
bool parse_cpu_total(const std::string &proc_stat, long long *total, long long *idle);
// utime + stime ticks from /proc/self/stat.
long long parse_self_ticks(const std::string &proc_self_stat);

long long read_net_rx(std::string *busiest = nullptr);
bool read_cpu_total(long long *total, long long *idle);
long long read_self_ticks();

// Throughput from cumulative byte counts sampled over time.
class RateMeter {
 public:
  void reset();
  void add(long long now_ms, long long cumulative_bytes);
  // Over the last `window_ms` (default 2 s); -1 before two samples exist.
  double mbps(long long window_ms = 2000) const;
  double average_mbps() const;  // since the first sample
  double peak_mbps() const { return peak_; }
  long long bytes() const;      // received since the first sample

 private:
  struct Sample {
    long long at, bytes;
  };
  std::deque<Sample> samples_;
  long long first_at_ = -1, first_bytes_ = 0;
  double peak_ = 0;
};

// "58.4 Mbps", "850 kbps"; "—" for a negative value.
std::string mbps_text(double mbps);
// "74.2 GB", "850 MB".
std::string bytes_text(long long bytes);

}  // namespace stats
