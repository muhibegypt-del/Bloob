#include "http.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>

namespace {

long long now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool wait_fd(int fd, short events, long long deadline) {
  for (;;) {
    long long left = deadline - now_ms();
    if (left <= 0) return false;
    pollfd p{fd, events, 0};
    int r = poll(&p, 1, static_cast<int>(left));
    if (r > 0) return true;
    if (r == 0) return false;
    if (errno != EINTR) return false;
  }
}

std::string lower(std::string s) {
  for (auto &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  return s;
}

// Decode a chunked transfer-encoded body. Returns false if malformed.
bool dechunk(const std::string &in, std::string &out) {
  size_t pos = 0;
  out.clear();
  while (pos < in.size()) {
    size_t eol = in.find("\r\n", pos);
    if (eol == std::string::npos) return false;
    unsigned long size = std::strtoul(in.substr(pos, eol - pos).c_str(), nullptr, 16);
    pos = eol + 2;
    if (size == 0) return true;
    // Compare against what is left, never `pos + size`: a huge size would wrap around.
    if (size > in.size() - pos || in.size() - pos - size < 2) return false;
    out.append(in, pos, size);
    pos += size;
    if (in.compare(pos, 2, "\r\n") != 0) return false;  // each chunk ends with CRLF
    pos += 2;
  }
  return false;
}

}  // namespace

std::string url_encode(const std::string &value) {
  static const char *hex = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += static_cast<char>(c);
    else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

HttpResponse http_request(const std::string &method, int port, const std::string &path, const std::string &body,
                          int timeout_ms) {
  HttpResponse res;
  long long deadline = now_ms() + timeout_ms;
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    res.error = "socket failed";
    return res;
  }
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) != 0 && errno != EINPROGRESS) {
    close(fd);
    res.error = "connection refused";
    return res;
  }
  if (!wait_fd(fd, POLLOUT, deadline)) {
    close(fd);
    res.error = "connect timed out";
    return res;
  }
  int soerr = 0;
  socklen_t len = sizeof soerr;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
  if (soerr != 0) {
    close(fd);
    res.error = "connection refused";
    return res;
  }

  std::string request = method + " " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nAccept: */*\r\n";
  if (method == "POST") {
    request += "Content-Type: application/json\r\nContent-Length: " + std::to_string(body.size()) + "\r\n";
  }
  request += "\r\n" + body;
  size_t sent = 0;
  while (sent < request.size()) {
    if (!wait_fd(fd, POLLOUT, deadline)) {
      close(fd);
      res.error = "send timed out";
      return res;
    }
    ssize_t n = send(fd, request.data() + sent, request.size() - sent, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      close(fd);
      res.error = "send failed";
      return res;
    }
    sent += static_cast<size_t>(n);
  }

  std::string raw;
  char buf[16384];
  for (;;) {
    if (!wait_fd(fd, POLLIN, deadline)) {
      close(fd);
      res.error = "timed out";
      return res;
    }
    ssize_t n = recv(fd, buf, sizeof buf, 0);
    if (n < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      close(fd);
      res.error = "receive failed";
      return res;
    }
    if (n == 0) break;
    raw.append(buf, static_cast<size_t>(n));
    if (raw.size() > 64u * 1024 * 1024) break;
  }
  close(fd);

  size_t head_end = raw.find("\r\n\r\n");
  if (head_end == std::string::npos || raw.compare(0, 5, "HTTP/") != 0) {
    res.error = "malformed response";
    return res;
  }
  std::string head = raw.substr(0, head_end);
  size_t sp = head.find(' ');
  res.status = sp == std::string::npos ? 0 : std::atoi(head.c_str() + sp + 1);
  bool chunked = false;
  size_t line = head.find("\r\n");
  while (line != std::string::npos) {
    size_t next = head.find("\r\n", line + 2);
    std::string h = head.substr(line + 2, next == std::string::npos ? std::string::npos : next - line - 2);
    size_t colon = h.find(':');
    if (colon != std::string::npos) {
      std::string name = lower(h.substr(0, colon));
      std::string value = h.substr(colon + 1);
      while (!value.empty() && value[0] == ' ') value.erase(0, 1);
      if (name == "content-type") res.content_type = value;
      if (name == "transfer-encoding" && lower(value).find("chunked") != std::string::npos) chunked = true;
    }
    line = next;
  }
  std::string payload = raw.substr(head_end + 4);
  if (chunked) {
    if (!dechunk(payload, res.body)) res.body = payload;
  } else {
    res.body.swap(payload);
  }
  if (res.status == 0) res.error = "malformed status";
  return res;
}
