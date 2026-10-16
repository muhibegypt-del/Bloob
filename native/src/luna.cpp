#include "luna.h"

#include <glib.h>
#include <webos-helpers/libhelpers.h>

#include <sys/prctl.h>

#include "clock.h"
#include "log.h"

// The record starts with the HContext so the pointer libhelpers hands back
// identifies the call whether it passes the context or its userdata.
struct Luna::Call {
  HContext ctx{};
  int id = 0;
  bool subscribe = false;
  Callback callback;
  Luna *owner = nullptr;
};

Luna &Luna::instance() {
  static Luna luna;
  return luna;
}

bool Luna::start(std::function<void()> wake) {
  wake_ = std::move(wake);
  // libhelpers registers its bus handle on the default GLib context; keep that
  // context running on its own thread so replies flow without blocking the UI.
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
  loop_ = loop;
  loop_thread_ = std::thread([loop] {
    prctl(PR_SET_NAME, "bl-luna", 0, 0, 0);
    g_main_loop_run(loop);
  });
  logf("luna", "GLib loop started");
  return true;
}

void Luna::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &entry : calls_) {
      if (entry.second->subscribe) HUnregisterServiceCallback(&entry.second->ctx);
    }
    calls_.clear();
  }
  if (loop_) {
    g_main_loop_quit(static_cast<GMainLoop *>(loop_));
    if (loop_thread_.joinable()) loop_thread_.join();
    g_main_loop_unref(static_cast<GMainLoop *>(loop_));
    loop_ = nullptr;
  }
  retired_.clear();
}

bool Luna::on_reply(void * /*handle*/, void *message, void *ctx) {
  auto *call = static_cast<Call *>(ctx);
  const char *payload = HLunaServiceMessage(static_cast<LSMessage *>(message));
  Luna *self = call->owner;
  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    if (!self->calls_.count(call->id)) return true; // cancelled or finished; record is retired, still valid
    self->queue_.emplace_back(call->id, payload ? payload : "{}");
  }
  if (self->wake_) self->wake_();
  return true;
}

int Luna::call(const std::string &uri, const std::string &payload, Callback callback, bool subscribe) {
  auto call = std::make_unique<Call>();
  Call *raw = call.get();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    raw->id = next_id_++;
    raw->subscribe = subscribe;
    raw->callback = std::move(callback);
    raw->owner = this;
    raw->ctx.callback = reinterpret_cast<LSFilterFunc>(&Luna::on_reply);
    raw->ctx.userdata = raw;
    raw->ctx.multiple = subscribe ? 1 : 0;
    raw->ctx.pub = 1;
    calls_[raw->id] = std::move(call);
  }
  int id = raw->id;
  if (HLunaServiceCall(uri.c_str(), payload.c_str(), &raw->ctx) != 0) {
    logf("luna", "call failed: %s", uri.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    retired_.push_back(std::move(calls_[id]));
    calls_.erase(id);
    return 0;
  }
  return id;
}

void Luna::cancel(int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = calls_.find(id);
  if (it == calls_.end()) return;
  if (it->second->subscribe) HUnregisterServiceCallback(&it->second->ctx);
  // Retire, never free, while the bus might still deliver a late reply.
  retired_.push_back(std::move(it->second));
  calls_.erase(it);
}

bool Luna::pump(int budget_ms) {
  long long start = budget_ms > 0 ? now_ms() : 0;
  for (;;) {
    std::pair<int, std::string> item;
    Callback callback;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty()) return false;
      item = std::move(queue_.front());
      queue_.pop_front();
      auto it = calls_.find(item.first);
      if (it != calls_.end()) {
        callback = it->second->callback;
        // One-shot calls are finished once their reply is delivered. The record
        // is retired rather than freed: libhelpers may still touch its context
        // after the callback returns on the GLib thread.
        if (!it->second->subscribe) {
          retired_.push_back(std::move(it->second));
          calls_.erase(it);
          if (retired_.size() > 512 && !retired_warned_) {
            retired_warned_ = true;
            logf("luna", "more than 512 retired call records this session");
          }
        }
      }
    }
    if (callback) {
      callback(item.second);
      delivered_++;
    }
    if (budget_ms > 0 && now_ms() - start >= budget_ms) {
      std::lock_guard<std::mutex> lock(mutex_);
      return !queue_.empty();
    }
  }
}

Luna::Stats Luna::stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  Stats s;
  s.live = calls_.size();
  s.retired = retired_.size();
  s.queued = queue_.size();
  s.delivered = delivered_;
  return s;
}

void Luna::inject(const std::string &payload) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &entry : calls_) {
      if (entry.second->subscribe) queue_.emplace_back(entry.first, payload);
    }
  }
  if (wake_) wake_();
}
