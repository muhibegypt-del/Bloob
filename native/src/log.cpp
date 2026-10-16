#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>

static FILE *g_file = nullptr;
static std::mutex g_mutex;
static std::deque<std::string> g_tail;

void log_init(const char *path) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_file = std::fopen(path, "w");
}

void logf(const char *tag, const char *fmt, ...) {
  char message[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof message, fmt, args);
  va_end(args);

  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  char line[1200];
  std::snprintf(line, sizeof line, "%6ld.%03ld [%s] %s", static_cast<long>(ts.tv_sec),
                ts.tv_nsec / 1000000, tag, message);

  std::lock_guard<std::mutex> lock(g_mutex);
  std::fprintf(stderr, "%s\n", line);
  if (g_file) {
    std::fprintf(g_file, "%s\n", line);
    std::fflush(g_file);
  }
  g_tail.emplace_back(line);
  if (g_tail.size() > 200) g_tail.pop_front();
}

void logf_limited(const char *key, long long interval_ms, const char *tag, const char *fmt, ...) {
  static std::mutex limit_mutex;
  static std::map<std::string, std::pair<long long, long long>> last;  // key -> (at, skipped)
  long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  long long skipped = 0;
  {
    std::lock_guard<std::mutex> lock(limit_mutex);
    auto it = last.find(key);
    if (it != last.end() && now - it->second.first < interval_ms) {
      it->second.second++;
      return;
    }
    if (it != last.end()) skipped = it->second.second;
    last[key] = {now, 0};
  }
  char message[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(message, sizeof message, fmt, args);
  va_end(args);
  if (skipped) logf(tag, "%s (%lld similar lines skipped)", message, skipped);
  else logf(tag, "%s", message);
}

std::vector<std::string> log_tail(size_t count) {
  std::lock_guard<std::mutex> lock(g_mutex);
  size_t start = g_tail.size() > count ? g_tail.size() - count : 0;
  return {g_tail.begin() + static_cast<long>(start), g_tail.end()};
}

std::string redact_uri(const std::string &uri) {
  auto scheme = uri.find("://");
  if (scheme == std::string::npos) return "<uri>";
  auto host_end = uri.find('/', scheme + 3);
  if (uri.compare(0, scheme, "file") == 0) return "file://<local>";
  return uri.substr(0, host_end == std::string::npos ? uri.size() : host_end);
}
