#pragma once
// Luna (webOS system service bus) client built on LG's libhelpers, as Kodi and
// Moonlight TV use it. Replies arrive on a GLib thread and are handed to the
// UI thread through pump(), so callbacks never race the renderer.
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Luna {
 public:
  using Callback = std::function<void(const std::string &payload)>;

  // `wake` is called (from any thread) when replies are queued.
  bool start(std::function<void()> wake);
  void stop();

  // Returns a call id (>0), or 0 when the call could not be sent.
  int call(const std::string &uri, const std::string &payload, Callback callback, bool subscribe = false);
  void cancel(int id);
  // UI thread: deliver queued replies to their callbacks until `budget_ms` has
  // passed (0: all). True if replies remain (R5a: bounded work per wake).
  bool pump(int budget_ms = 0);
  // Test hook: deliver `payload` to every active subscription as if the bus sent it.
  void inject(const std::string &payload);

  static Luna &instance();

  // For the inspector. Retired records are finished or cancelled calls kept
  // alive because the closed libhelpers library may still touch their context;
  // nothing proves when it stops, so they are counted, not freed (study A4).
  struct Stats {
    size_t live = 0, retired = 0, queued = 0;
    long long delivered = 0;
  };
  Stats stats();

 private:
  struct Call;
  static bool on_reply(void *handle, void *message, void *ctx);

  std::function<void()> wake_;
  std::thread loop_thread_;
  void *loop_ = nullptr;
  std::mutex mutex_;
  int next_id_ = 1;
  std::map<int, std::unique_ptr<Call>> calls_;
  std::vector<std::unique_ptr<Call>> retired_;
  std::deque<std::pair<int, std::string>> queue_;
  long long delivered_ = 0;
  bool retired_warned_ = false;
};
