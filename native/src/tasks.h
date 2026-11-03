#pragma once
// Worker threads for blocking work (HTTP, image fetching, decoding). A job runs
// on a worker; its completion runs on the UI thread when the main loop calls
// pump(). A job whose owner has gone is skipped.
//
// R5a/R5b (Elements of A resource study C2, C5, C12):
//  - three priority lanes (visible, near, prefetch); within a lane, newest
//    first (newest focus wins) unless the caller asks for order;
//  - the wake is a notification only: the completion queue carries the work,
//    and the app coalesces wakes;
//  - pump() works to a time budget and reports whether work remains;
//  - workers are named threads with an explicit stack size.
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Tasks {
 public:
  using Work = std::function<std::function<void()>()>;  // returns the UI-thread completion (may be empty)

  // `name` labels the threads ("bl-api" -> bl-api-1, bl-api-2 …).
  void start(int threads, std::function<void()> wake, const char *name = "bl-work", size_t stack_bytes = 512 * 1024);
  void stop();
  // `owner`: the job is dropped (before running, and before completing) once it expires.
  // `lane`: 0 visible, 1 near, 2 prefetch. `lifo`: newest first within the lane.
  void run(Work work, const std::shared_ptr<void> &owner, int lane, bool lifo);
  void run(Work work, const std::shared_ptr<void> &owner = nullptr, bool urgent = true) {
    run(std::move(work), owner, 0, urgent);
  }
  // Runs completions until `budget_ms` has passed (0: all). Returns how many
  // ran; `more` (optional) says whether any remain.
  size_t pump(int budget_ms = 0, bool *more = nullptr);
  // From any thread: run `completion` on the UI thread at the next pump (the
  // playback engine's threads use this for pipeline events). Dropped once `owner` expires.
  void post(std::function<void()> completion, const std::shared_ptr<void> &owner = nullptr);
  size_t pending() const;

  struct Stats {
    size_t queued[3] = {0, 0, 0};
    int busy = 0;
    size_t completions_waiting = 0;
    long long ran = 0, dropped_owner = 0, dropped_stop = 0;
  };
  Stats stats() const;

 private:
  struct Job {
    Work work;
    std::weak_ptr<void> owner;
    bool owned;
  };
  void loop();
  static void *entry(void *self);

  std::vector<unsigned long> threads_;  // pthread_t
  std::string name_;
  std::atomic<int> started_{0};
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<Job> lanes_[3];
  std::deque<std::pair<std::weak_ptr<void>, std::pair<bool, std::function<void()>>>> done_;
  std::function<void()> wake_;
  bool stopping_ = false;
  int busy_ = 0;
  long long ran_ = 0, dropped_owner_ = 0, dropped_stop_ = 0;
};
