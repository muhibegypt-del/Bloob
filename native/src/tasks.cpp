#include "tasks.h"

void Tasks::start(int threads, std::function<void()> wake) {
  wake_ = std::move(wake);
  for (int i = 0; i < threads; i++) threads_.emplace_back([this] { loop(); });
}

void Tasks::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
    queue_.clear();
  }
  cv_.notify_all();
  for (auto &t : threads_) if (t.joinable()) t.join();
  threads_.clear();
  done_.clear();
}

void Tasks::run(Work work, const std::shared_ptr<void> &owner, bool urgent) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    Job job{std::move(work), std::weak_ptr<void>(owner), owner != nullptr};
    if (urgent) queue_.push_front(std::move(job));
    else queue_.push_back(std::move(job));
  }
  cv_.notify_one();
}

size_t Tasks::pending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size() + static_cast<size_t>(busy_);
}

void Tasks::loop() {
  for (;;) {
    Job job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
      if (stopping_) return;
      job = std::move(queue_.front());
      queue_.pop_front();
      if (job.owned && job.owner.expired()) continue;
      busy_++;
    }
    std::function<void()> complete = job.work ? job.work() : std::function<void()>{};
    {
      std::lock_guard<std::mutex> lock(mutex_);
      busy_--;
      if (complete) done_.push_back({job.owner, {job.owned, std::move(complete)}});
    }
    if (complete && wake_) wake_();
  }
}

void Tasks::pump() {
  std::vector<std::pair<std::weak_ptr<void>, std::pair<bool, std::function<void()>>>> ready;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ready.swap(done_);
  }
  for (auto &item : ready) {
    if (item.second.first && item.first.expired()) continue;
    item.second.second();
  }
}
