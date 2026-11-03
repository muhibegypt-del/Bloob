#pragma once
// Blocking HTTP/1.1 client for the app's own local service (127.0.0.1 only).
// Call it from worker threads, never the UI thread.
#include <string>

struct HttpResponse {
  int status = 0;            // 0 = connection failed or timed out
  std::string content_type;
  std::string body;
  std::string error;         // set when status is 0
};

HttpResponse http_request(const std::string &method, int port, const std::string &path, const std::string &body = "",
                          int timeout_ms = 20000);

// Percent-encode for a query value.
std::string url_encode(const std::string &value);
