#pragma once
// Worker threads for blocking work (HTTP, image decoding). A job runs on a
// worker; its completion runs on the UI thread when the main loop calls
// pump(). Jobs queued later run first (newest focus wins), and a job whose
// owner has gone is skipped.
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class Tasks {
 public:
  using Work = std::function<std::function<void()>()>;  // returns the UI-thread completion (may be empty)

  void start(int threads, std::function<void()> wake);
  void stop();
  // `owner`: the job is dropped (before running, and before completing) once it expires.
  void run(Work work, const std::shared_ptr<void> &owner = nullptr, bool urgent = true);
  void pump();
  size_t pending() const;

 private:
  struct Job {
    Work work;
    std::weak_ptr<void> owner;
    bool owned;
  };
  void loop();

  std::vector<std::thread> threads_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<Job> queue_;
  std::vector<std::pair<std::weak_ptr<void>, std::pair<bool, std::function<void()>>>> done_;
  std::function<void()> wake_;
  bool stopping_ = false;
  int busy_ = 0;
};
