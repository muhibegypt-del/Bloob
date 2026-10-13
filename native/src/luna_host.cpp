// Desktop stand-in for the webOS bus, used only by the host UI test build.
// Answers the calls Bloob makes with plausible replies; media events are
// injected by the test script through Luna::inject().
#include "luna.h"

#include "log.h"

struct Luna::Call {
  int id = 0;
  bool subscribe = false;
  Callback callback;
};

Luna &Luna::instance() {
  static Luna luna;
  return luna;
}

bool Luna::start(std::function<void()> wake) {
  wake_ = std::move(wake);
  logf("luna", "host stand-in bus");
  return true;
}

void Luna::stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  calls_.clear();
}

bool Luna::on_reply(void *, void *, void *) { return true; }

int Luna::call(const std::string &uri, const std::string &payload, Callback callback, bool subscribe) {
  std::string reply = "{\"returnValue\":true}";
  if (uri.find("getSystemInfo") != std::string::npos)
    reply = "{\"returnValue\":true,\"modelName\":\"Host preview\",\"sdkVersion\":\"7.3.0\",\"firmwareVersion\":\"0.0.0\"}";
  else if (uri.find("com.webos.media/load") != std::string::npos)
    reply = "{\"returnValue\":true,\"mediaId\":\"_host_media\"}";
  else if (subscribe)
    reply = "{\"returnValue\":true,\"subscribed\":true}";
  int id;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    id = next_id_++;
    auto call = std::make_unique<Call>();
    call->id = id;
    call->subscribe = subscribe;
    call->callback = std::move(callback);
    calls_[id] = std::move(call);
    queue_.emplace_back(id, reply);
  }
  logf("luna", "host call %s", uri.c_str());
  if (wake_) wake_();
  return id;
}

void Luna::cancel(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  calls_.erase(id);
}

void Luna::pump() {
  std::vector<std::pair<int, std::string>> ready;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ready.swap(queue_);
  }
  for (auto &item : ready) {
    Callback callback;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = calls_.find(item.first);
      if (it == calls_.end()) continue;
      callback = it->second->callback;
      if (!it->second->subscribe) calls_.erase(it);
    }
    if (callback) callback(item.second);
  }
}

void Luna::inject(const std::string &payload) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &entry : calls_)
      if (entry.second->subscribe) queue_.emplace_back(entry.first, payload);
  }
  if (wake_) wake_();
}
