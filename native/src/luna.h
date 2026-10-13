#pragma once
// Luna (webOS system service bus) client built on LG's libhelpers, as Kodi and
// Moonlight TV use it. Replies arrive on a GLib thread and are handed to the
// UI thread through pump(), so callbacks never race the renderer.
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
  // UI thread: deliver queued replies to their callbacks.
  void pump();
  // Test hook: deliver `payload` to every active subscription as if the bus sent it.
  void inject(const std::string &payload);

  static Luna &instance();

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
  std::vector<std::pair<int, std::string>> queue_;
};
