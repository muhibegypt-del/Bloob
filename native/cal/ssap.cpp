#include "ssap.h"

#include <cstdlib>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>

namespace cal {

static std::string slurp(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

static std::string b64(const unsigned char *d, size_t n) {
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string o;
  for (size_t i = 0; i < n; i += 3) {
    unsigned v = d[i] << 16 | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
    o += t[v >> 18 & 63];
    o += t[v >> 12 & 63];
    o += i + 1 < n ? t[v >> 6 & 63] : '=';
    o += i + 2 < n ? t[v & 63] : '=';
  }
  return o;
}

void Ssap::close() {
  if (fd_ >= 0) ::close(fd_);
  fd_ = -1;
}

bool Ssap::read_exact(void *buf, size_t n, int timeout_ms) {
  auto *p = static_cast<unsigned char *>(buf);
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (n) {
    int left = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
    if (left <= 0) return false;
    pollfd pf{fd_, POLLIN, 0};
    if (poll(&pf, 1, left) <= 0) return false;
    ssize_t r = ::read(fd_, p, n);
    if (r <= 0) return false;
    p += r;
    n -= static_cast<size_t>(r);
  }
  return true;
}

bool Ssap::send_text(const std::string &text) {
  // A client frame: FIN + text, masked as the protocol requires.
  std::string f;
  f += static_cast<char>(0x81);
  size_t n = text.size();
  if (n < 126) {
    f += static_cast<char>(0x80 | n);
  } else if (n < 65536) {
    f += static_cast<char>(0x80 | 126);
    f += static_cast<char>(n >> 8);
    f += static_cast<char>(n & 0xff);
  } else {
    f += static_cast<char>(0x80 | 127);
    for (int i = 7; i >= 0; i--) f += static_cast<char>((static_cast<unsigned long long>(n) >> (8 * i)) & 0xff);
  }
  static std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
  unsigned char mask[4];
  for (auto &m : mask) m = static_cast<unsigned char>(rng());
  f.append(reinterpret_cast<char *>(mask), 4);
  size_t start = f.size();
  f += text;
  for (size_t i = 0; i < n; i++) f[start + i] = static_cast<char>(f[start + i] ^ mask[i & 3]);
  const char *p = f.data();
  size_t left = f.size();
  while (left) {
    ssize_t w = ::write(fd_, p, left);
    if (w <= 0) return false;
    p += w;
    left -= static_cast<size_t>(w);
  }
  return true;
}

bool Ssap::receive_text(std::string *text, int timeout_ms) {
  text->clear();
  for (;;) {
    unsigned char h[2];
    if (!read_exact(h, 2, timeout_ms)) return false;
    bool fin = h[0] & 0x80;
    int op = h[0] & 0x0f;
    unsigned long long n = h[1] & 0x7f;
    if (n == 126) {
      unsigned char e[2];
      if (!read_exact(e, 2, timeout_ms)) return false;
      n = static_cast<unsigned long long>(e[0]) << 8 | e[1];
    } else if (n == 127) {
      unsigned char e[8];
      if (!read_exact(e, 8, timeout_ms)) return false;
      n = 0;
      for (unsigned char c : e) n = n << 8 | c;
    }
    unsigned char mask[4] = {0, 0, 0, 0};
    bool masked = h[1] & 0x80;
    if (masked && !read_exact(mask, 4, timeout_ms)) return false;
    if (n > (16u << 20)) return false;
    std::string body(static_cast<size_t>(n), '\0');
    if (n && !read_exact(&body[0], static_cast<size_t>(n), timeout_ms)) return false;
    if (masked)
      for (size_t i = 0; i < body.size(); i++) body[i] = static_cast<char>(body[i] ^ mask[i & 3]);
    if (op == 0x8) return false;  // closed
    if (op == 0x9) {              // ping: answer, keep waiting
      std::string pong;
      pong += static_cast<char>(0x8a);
      pong += static_cast<char>(0x80);
      pong.append(4, '\0');
      if (::write(fd_, pong.data(), pong.size()) < 0) return false;
      continue;
    }
    if (op == 0xa) continue;
    *text += body;
    if (fin) return true;
  }
}

bool Ssap::connect(const std::string &manifest_path, const std::string &key_path, std::string *error) {
  close();
  std::string manifest = slurp(manifest_path);
  bool ok = false;
  Json::parse(manifest, &ok);
  if (!ok) {
    *error = "the signed manifest is missing or unreadable: " + manifest_path;
    return false;
  }
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in a{};
  a.sin_family = AF_INET;
  // The TV's own port; BLOOB_SSAP_PORT moves it for the stand-in TV on a desktop.
  const char *port = getenv("BLOOB_SSAP_PORT");
  a.sin_port = htons(static_cast<uint16_t>(port ? atoi(port) : 3000));
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (fd_ < 0 || ::connect(fd_, reinterpret_cast<sockaddr *>(&a), sizeof a) != 0) {
    *error = "the TV's second-screen socket (port 3000) did not answer";
    close();
    return false;
  }
  unsigned char nonce[16];
  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
  for (auto &c : nonce) c = static_cast<unsigned char>(rng());
  std::string req = "GET / HTTP/1.1\r\nHost: 127.0.0.1:3000\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + b64(nonce, 16) +
                    "\r\nSec-WebSocket-Version: 13\r\n\r\n";
  if (::write(fd_, req.data(), req.size()) != static_cast<ssize_t>(req.size())) {
    *error = "could not write to the socket";
    close();
    return false;
  }
  std::string head;
  char c;
  while (head.find("\r\n\r\n") == std::string::npos) {
    if (!read_exact(&c, 1, 5000) || head.size() > 8192) {
      *error = "the socket did not accept a websocket";
      close();
      return false;
    }
    head += c;
  }
  if (head.find(" 101") == std::string::npos) {
    *error = "the socket refused the websocket: " + head.substr(0, head.find("\r\n"));
    close();
    return false;
  }
  std::string key;
  {
    bool kok = false;
    Json k = Json::parse(slurp(key_path), &kok);
    if (kok) key = k["client-key"].as_string();
  }
  std::string reg = "{\"type\":\"register\",\"id\":\"register\",\"payload\":{" +
                    (key.empty() ? std::string() : "\"client-key\":" + Json::quote(key) + ",") +
                    "\"forcePairing\":false,\"pairingType\":\"PROMPT\",\"manifest\":" + manifest + "}}";
  if (!send_text(reg)) {
    *error = "could not register";
    close();
    return false;
  }
  if (key.empty()) std::printf("  The TV is asking to allow Bloob: press OK on the remote (2 minutes).\n");
  for (;;) {
    std::string msg;
    if (!receive_text(&msg, 120000)) {
      *error = "the TV did not allow the connection (no answer to its prompt)";
      close();
      return false;
    }
    Json m = Json::parse(msg);
    std::string type = m["type"].as_string();
    if (type == "registered") {
      std::string k = m["payload"]["client-key"].as_string();
      if (!k.empty() && k != key) {
        std::string tmp = key_path + ".tmp";
        int f = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (f >= 0) {
          std::string body = "{\"client-key\":" + Json::quote(k) + "}";
          bool written = ::write(f, body.data(), body.size()) == static_cast<ssize_t>(body.size());
          ::close(f);
          if (written) ::rename(tmp.c_str(), key_path.c_str());
          ::chmod(key_path.c_str(), 0600);
        }
      }
      return true;
    }
    if (type == "error") {
      *error = "the TV refused: " + m["error"].as_string();
      close();
      return false;
    }
  }
}

bool Ssap::request(const std::string &uri, const std::string &payload, Json *reply, std::string *error, int timeout_ms) {
  if (fd_ < 0) {
    *error = "not connected";
    return false;
  }
  std::string id = std::to_string(next_id_++);
  if (!send_text("{\"id\":\"" + id + "\",\"type\":\"request\",\"uri\":" + Json::quote(uri) + ",\"payload\":" + payload + "}")) {
    *error = "the socket closed";
    close();
    return false;
  }
  for (;;) {
    std::string msg;
    if (!receive_text(&msg, timeout_ms)) {
      *error = "no answer from the TV";
      close();
      return false;
    }
    Json m = Json::parse(msg);
    if (m["id"].as_string() != id) continue;
    *reply = m["payload"];
    if (m["type"].as_string() == "error") {
      *error = m["error"].as_string();
      return false;
    }
    return true;
  }
}

}  // namespace cal
