#include "meter.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace cal {

bool parse_spotread(const std::string &text, Vec3 *XYZ) {
  size_t p = text.find("Result is XYZ:");
  if (p == std::string::npos) return false;
  double x, y, z;
  if (std::sscanf(text.c_str() + p, "Result is XYZ: %lf %lf %lf", &x, &y, &z) != 3) return false;
  *XYZ = {x, y, z};
  return true;
}

static std::vector<std::string> argv_for(const MeterConfig &c, bool one_shot) {
  std::vector<std::string> a;
  if (!c.loader.empty()) a.push_back(c.loader);
  a.push_back(c.spotread);
  a.push_back("-e");
  a.push_back("-x");
  if (one_shot) a.push_back("-O");
  // As PGenerator+ runs spotread for an OLED (research/PGenerator-Plus-and-LG-for-Bloob.md §3).
  if (c.no_init_cal) a.push_back("-N");
  if (!c.display.empty()) {
    a.push_back("-y");
    a.push_back(c.display);
  }
  if (!c.refresh_hz.empty()) {
    a.push_back("-Y");
    a.push_back("R:" + c.refresh_hz);
  }
  if (!c.ccss.empty()) {
    a.push_back("-X");
    a.push_back(c.ccss);
  }
  size_t s = 0;
  while (s < c.extra.size()) {
    size_t e = c.extra.find(' ', s);
    if (e == std::string::npos) e = c.extra.size();
    if (e > s) a.push_back(c.extra.substr(s, e - s));
    s = e + 1;
  }
  return a;
}

static void exec_spotread(const MeterConfig &c, bool one_shot) {
  if (!c.libs.empty()) setenv("LD_LIBRARY_PATH", c.libs.c_str(), 1);
  auto a = argv_for(c, one_shot);
  std::vector<char *> v;
  for (auto &s : a) v.push_back(&s[0]);
  v.push_back(nullptr);
  execv(v[0], v.data());
  _exit(127);
}

void Meter::close() {
  if (pid_ > 0) {
    if (fd_ >= 0 && ::write(fd_, "q", 1) < 0) {
    }
    for (int i = 0; i < 20; i++) {
      if (waitpid(pid_, nullptr, WNOHANG) == pid_) {
        pid_ = -1;
        break;
      }
      usleep(100000);
    }
    if (pid_ > 0) {
      kill(pid_, SIGTERM);
      waitpid(pid_, nullptr, 0);
    }
  }
  pid_ = -1;
  if (fd_ >= 0) ::close(fd_);
  fd_ = -1;
  buf_.clear();
}

bool Meter::wait_for(const std::string &marker, int timeout_ms, std::string *seen) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  for (;;) {
    size_t p = buf_.find(marker);
    if (p != std::string::npos) {
      size_t end = buf_.find('\n', p);
      end = end == std::string::npos ? buf_.size() : end + 1;
      if (seen) *seen = buf_.substr(0, end);
      buf_.erase(0, end);
      return true;
    }
    int left = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
    if (left <= 0) return false;
    pollfd pf{fd_, POLLIN, 0};
    if (poll(&pf, 1, left) <= 0) return false;
    char tmp[4096];
    ssize_t r = ::read(fd_, tmp, sizeof tmp);
    if (r <= 0) return false;
    buf_.append(tmp, static_cast<size_t>(r));
    if (buf_.size() > (1 << 20)) buf_.erase(0, buf_.size() - 4096);
  }
}

bool Meter::open(const MeterConfig &c, std::string *error) {
  close();
  cfg_ = c;
  fallback_ = false;
  int m = posix_openpt(O_RDWR | O_NOCTTY);
  if (m >= 0 && grantpt(m) == 0 && unlockpt(m) == 0) {
    const char *slave = ptsname(m);
    std::string slave_name = slave ? slave : "";
    pid_t pid = slave ? fork() : -1;
    if (pid == 0) {
      setsid();
      int s = ::open(slave_name.c_str(), O_RDWR);
      if (s < 0) _exit(126);
      dup2(s, 0);
      dup2(s, 1);
      dup2(s, 2);
      if (s > 2) ::close(s);
      ::close(m);
      exec_spotread(c, false);
    }
    if (pid > 0) {
      fd_ = m;
      pid_ = pid;
      std::string seen;
      // spotread's prompt ends with "any other key to take a reading".
      if (wait_for("take a reading", 60000, &seen)) return true;
      close();
    } else {
      ::close(m);
    }
  } else if (m >= 0) {
    ::close(m);
  }
  // One spotread per reading, as the probe app does.
  fallback_ = true;
  Vec3 x;
  if (read_one_shot(&x, error)) return true;
  return false;
}

bool Meter::read_one_shot(Vec3 *XYZ, std::string *error) {
  int out[2], in[2];
  if (pipe(out) != 0 || pipe(in) != 0) {
    *error = "pipe failed";
    return false;
  }
  pid_t pid = fork();
  if (pid == 0) {
    dup2(in[0], 0);
    dup2(out[1], 1);
    dup2(out[1], 2);
    ::close(in[1]);
    ::close(out[0]);
    exec_spotread(cfg_, true);
  }
  ::close(in[0]);
  ::close(out[1]);
  if (::write(in[1], "\n", 1) < 0) {
  }
  ::close(in[1]);
  std::string text;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
  for (;;) {
    int left = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
    pollfd pf{out[0], POLLIN, 0};
    if (left <= 0 || poll(&pf, 1, left) <= 0) break;
    char tmp[4096];
    ssize_t r = ::read(out[0], tmp, sizeof tmp);
    if (r <= 0) break;
    text.append(tmp, static_cast<size_t>(r));
  }
  ::close(out[0]);
  kill(pid, SIGTERM);
  waitpid(pid, nullptr, 0);
  if (parse_spotread(text, XYZ)) return true;
  size_t nl = text.find_last_not_of("\r\n ");
  *error = "spotread gave no reading: " + text.substr(text.size() > 300 ? text.size() - 300 : 0, nl == std::string::npos ? 0 : 300);
  return false;
}

bool Meter::read(Vec3 *XYZ, std::string *error) {
  if (fallback_) return read_one_shot(XYZ, error);
  return read_session(XYZ, error);  // a stuck session is restarted by TvDevice, on a white square
}

bool Meter::read_session(Vec3 *XYZ, std::string *error) {
  if (pid_ <= 0 || ::write(fd_, " ", 1) != 1) {
    *error = "the meter session is closed";
    return false;
  }
  std::string seen;
  if (!wait_for("Result is XYZ:", 45000, &seen) || !parse_spotread(seen.substr(seen.rfind("Result is XYZ:")), XYZ)) {
    // Say what spotread said instead, so a stuck meter is never silent.
    std::string tail = seen.empty() ? buf_ : seen;
    for (char &c : tail)
      if (c == '\r' || c == '\n') c = ' ';
    *error = "spotread gave no reading within 45 s; it said: \"" + tail.substr(tail.size() > 300 ? tail.size() - 300 : 0) + "\"";
    return false;
  }
  // Ready for the next key.
  if (!wait_for("take a reading", 30000, nullptr)) {
    *error = "spotread did not ask for the next reading";
    return false;
  }
  return true;
}

}  // namespace cal
