#include "alloc.h"

#include <cstdlib>
#include <new>

AllocStats &alloc_stats() {
  static AllocStats stats;
  return stats;
}

static void *counted(std::size_t size) {
  AllocStats &s = alloc_stats();
  s.count.fetch_add(1, std::memory_order_relaxed);
  s.bytes.fetch_add(static_cast<long long>(size), std::memory_order_relaxed);
  void *p = std::malloc(size ? size : 1);
  if (!p) throw std::bad_alloc();
  return p;
}

void *operator new(std::size_t size) { return counted(size); }
void *operator new[](std::size_t size) { return counted(size); }
void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return counted(size);
  } catch (...) {
    return nullptr;
  }
}
void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return counted(size);
  } catch (...) {
    return nullptr;
  }
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { std::free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { std::free(p); }
