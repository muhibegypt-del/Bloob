#include "api.h"

#include "http.h"
#include "log.h"

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
        HttpResponse res = http_request(method, port, path, body, 30000);
        auto result = std::make_shared<ApiResult>();
        result->status = res.status;
        if (res.status == 0) {
          result->error = "Bloob's background service is not responding.";
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
        return [callback, result] { callback(*result); };
      },
      owner);
}
