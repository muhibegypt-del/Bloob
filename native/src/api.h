#pragma once
// Client for Bloob's local service (catalogue, search, sources, AllDebrid,
// artwork, watch progress). Requests run on worker threads; callbacks run on
// the UI thread, and are skipped if their owner has gone.
#include <functional>
#include <memory>
#include <string>

#include "json.h"
#include "tasks.h"

struct ApiResult {
  Json json;
  int status = 0;
  std::string error;  // plain-language message when !ok()
  bool ok() const { return status >= 200 && status < 300; }
};

class Api {
 public:
  using Callback = std::function<void(const ApiResult &)>;

  void init(Tasks *tasks) { tasks_ = tasks; }
  void set_port(int port) { port_ = port; }
  int port() const { return port_; }
  bool ready() const { return port_ > 0; }

  void get(const std::string &path, Callback callback, const std::shared_ptr<void> &owner);
  void post(const std::string &path, const std::string &body, Callback callback, const std::shared_ptr<void> &owner);
  // Fire and forget (progress saves).
  void post(const std::string &path, const std::string &body);

  // Blocking, for worker threads (image fetches).
  std::string fetch_bytes(const std::string &path, int *status) const;

 private:
  void request(const std::string &method, const std::string &path, const std::string &body, Callback callback,
               const std::shared_ptr<void> &owner);
  Tasks *tasks_ = nullptr;
  int port_ = 0;
};
