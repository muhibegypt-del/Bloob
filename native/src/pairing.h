#pragma once
// AllDebrid pairing panel: asks the service for a PIN, shows it with the
// address to enter it at, checks every five seconds and renews an expired PIN.
// The key itself never reaches the app; the service stores it.
#include <functional>
#include <memory>
#include <string>

#include "keys.h"

class Pairing {
 public:
  // `done(true)` once paired; `done(false)` if the viewer cancels.
  void start(std::function<void(bool)> done);
  bool active() const { return active_; }
  std::string spoken() const;
  void cancel();
  bool key(Key key);
  void update();
  void draw();

 private:
  void request_pin();
  void check();

  bool active_ = false;
  std::function<void(bool)> done_;
  std::string pin_, url_, error_;
  long long expires_at_ = 0;
  long long next_check_ = 0;
  bool checking_ = false;
  std::shared_ptr<int> alive_;
};
