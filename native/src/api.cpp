#include "api.h"

#include "clock.h"

#include "http.h"
#include "log.h"
#include "ui.h"

void Api::get(const std::string &path, Callback callback, const std::shared_ptr<void> &owner) {
  request("GET", path, "", std::move(callback), owner);
}

void Api::post(const std::string &path, const std::string &body, Callback callback, const std::shared_ptr<void> &owner) {
  request("POST", path, body, std::move(callback), owner);
}

void Api::post(const std::string &path, const std::string &body) {
  request("POST", path, body, nullptr, nullptr);
}

std::string Api::fetch_bytes(const std::string &path, int *status) const {
  HttpResponse res = http_request("GET", port_, path, "", 20000);
  if (status) *status = res.status;
  return res.status == 200 ? std::move(res.body) : std::string();
}

void Api::request(const std::string &method, const std::string &path, const std::string &body, Callback callback,
                  const std::shared_ptr<void> &owner) {
  int port = port_;
  std::string where = path.substr(0, path.find('?'));
  tasks_->run(
      [=]() -> std::function<void()> {
        long long started = now_ms();
        HttpResponse res = http_request(method, port, path, body, 30000);
        long long took = now_ms() - started;
        if (took > 20000) logf("api", "%s %s slow: %lld ms", method.c_str(), where.c_str(), took);
        auto result = std::make_shared<ApiResult>();
        result->status = res.status;
        if (res.status == 0) {
          // The service is local, so these say what happened to Bloob, not to the internet.
          bool timeout = res.error.find("timed out") != std::string::npos;
          result->error = timeout ? "Bloob's background service is taking too long to answer. Try again."
                                  : "Bloob's background service is not responding.";
          logf("api", "%s %s failed: %s", method.c_str(), where.c_str(), res.error.c_str());
        } else {
          bool ok = false;
          result->json = Json::parse(res.body, &ok);
          if (!result->ok()) {
            result->error = result->json["error"].as_string("Something went wrong (" + std::to_string(res.status) + ").");
            logf("api", "%s %s -> %d %s", method.c_str(), where.c_str(), res.status, result->error.c_str());
          } else if (!ok) {
            result->status = 0;
            result->error = "The service sent an unreadable reply.";
          }
        }
        if (!callback) return {};
        return [callback, result] {
          // Upstream failures while the TV is offline: say so plainly (read on the UI thread).
          if (!result->ok() && result->status >= 500 && !ctx().online)
            result->error = "You're offline. Check the TV's internet connection.";
          callback(*result);
        };
      },
      owner);
}
