#include "tasks.h"

#include <pthread.h>
#include <sys/prctl.h>

#include "clock.h"
#include "log.h"

void *Tasks::entry(void *self) {
  auto *tasks = static_cast<Tasks *>(self);
  // prctl, not pthread_setname_np: the TV build may import only GLIBC_2.4 symbols.
  int n = ++tasks->started_;
  std::string label = tasks->name_ + "-" + std::to_string(n);
  prctl(PR_SET_NAME, label.c_str(), 0, 0, 0);
  tasks->loop();
  return nullptr;
}

void Tasks::start(int threads, std::function<void()> wake, const char *name, size_t stack_bytes) {
  wake_ = std::move(wake);
  name_ = name ? name : "bl-work";
  for (int i = 0; i < threads; i++) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_bytes) pthread_attr_setstacksize(&attr, stack_bytes);
    pthread_t t;
    if (pthread_create(&t, &attr, &Tasks::entry, this) == 0) threads_.push_back(static_cast<unsigned long>(t));
    else logf("tasks", "%s: could not start a worker", name_.c_str());
    pthread_attr_destroy(&attr);
  }
}

void Tasks::stop() {
  size_t dropped = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
    for (auto &lane : lanes_) {
      dropped += lane.size();
      lane.clear();
    }
    dropped_stop_ += static_cast<long long>(dropped);
  }
  cv_.notify_all();
  for (unsigned long t : threads_) pthread_join(static_cast<pthread_t>(t), nullptr);
  threads_.clear();
  done_.clear();
  if (dropped) logf("tasks", "%s: %zu queued jobs dropped at stop", name_.c_str(), dropped);
}

void Tasks::run(Work work, const std::shared_ptr<void> &owner, int lane, bool lifo) {
  if (lane < 0) lane = 0;
  if (lane > 2) lane = 2;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    Job job{std::move(work), std::weak_ptr<void>(owner), owner != nullptr};
    if (lifo) lanes_[lane].push_front(std::move(job));
    else lanes_[lane].push_back(std::move(job));
  }
  cv_.notify_one();
}

size_t Tasks::pending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lanes_[0].size() + lanes_[1].size() + lanes_[2].size() + static_cast<size_t>(busy_);
}

Tasks::Stats Tasks::stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  Stats s;
  for (int i = 0; i < 3; i++) s.queued[i] = lanes_[i].size();
  s.busy = busy_;
  s.completions_waiting = done_.size();
  s.ran = ran_;
  s.dropped_owner = dropped_owner_;
  s.dropped_stop = dropped_stop_;
  return s;
}

void Tasks::loop() {
  for (;;) {
    Job job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return stopping_ || !lanes_[0].empty() || !lanes_[1].empty() || !lanes_[2].empty(); });
      if (stopping_) return;
      auto &lane = !lanes_[0].empty() ? lanes_[0] : !lanes_[1].empty() ? lanes_[1] : lanes_[2];
      job = std::move(lane.front());
      lane.pop_front();
      if (job.owned && job.owner.expired()) {
        dropped_owner_++;
        continue;
      }
      busy_++;
    }
    std::function<void()> complete = job.work ? job.work() : std::function<void()>{};
    // Test before the move: a moved-from std::function is empty, and testing it
    // afterwards (as 2.3 did) meant the UI thread was never woken for a
    // completion. Replies then waited for an unrelated wake, up to a second.
    bool has_completion = static_cast<bool>(complete);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      busy_--;
      ran_++;
      if (has_completion) done_.push_back({job.owner, {job.owned, std::move(complete)}});
    }
    if (has_completion && wake_) wake_();
  }
}

size_t Tasks::pump(int budget_ms, bool *more) {
  long long start = budget_ms > 0 ? now_ms() : 0;
  size_t ran = 0;
  if (more) *more = false;
  for (;;) {
    std::pair<std::weak_ptr<void>, std::pair<bool, std::function<void()>>> item;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (done_.empty()) return ran;
      item = std::move(done_.front());
      done_.pop_front();
    }
    if (!(item.second.first && item.first.expired())) {
      item.second.second();
      ran++;
    }
    if (budget_ms > 0 && now_ms() - start >= budget_ms) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (more) *more = !done_.empty();
      return ran;
    }
  }
}
