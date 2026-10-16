#pragma once
// One monotonic millisecond clock for the whole app, 64-bit so it never wraps
// (SDL_GetTicks is 32-bit and wraps after 49.7 days). Safe on any thread.
#include <chrono>

inline long long now_ms() {
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}
