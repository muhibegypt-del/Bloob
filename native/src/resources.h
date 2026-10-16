#pragma once
// Resource ownership and memory (R5b). Pure: no SDL, unit-tested.
//
// Modelled on the mechanisms in the Elements of A resource study (M3-M8), as
// Bloob's own design:
//  - one GPU budget for every texture the app creates, with fixed surfaces
//    reserved first, and admission checked BEFORE a texture is created;
//  - resources carry a relevance (visible, near, prefetch) and the frame they
//    were last drawn; eviction takes the least relevant first, then the oldest,
//    and never a resource drawn in the last `min_age` frames;
//  - one eviction per admission attempt, so pressure never becomes a burst of
//    destroys and re-uploads;
//  - CPU memory (decoded pixels) is counted in its own domain.
#include <atomic>
#include <cstddef>
#include <map>
#include <string>

enum class Relevance { Visible = 0, Near = 1, Prefetch = 2 };

// Where GPU bytes go, for accounting and the inspector.
enum class GpuClass { Surfaces = 0, Images, Shapes, Glyphs, Icons, Layers, Count };
const char *gpu_class_name(GpuClass c);

class GpuBudget {
 public:
  void set_limit(size_t bytes) { limit_ = bytes; }
  size_t limit() const { return limit_; }
  size_t used() const {
    size_t total = 0;
    for (size_t v : used_) total += v;
    return total;
  }
  size_t used(GpuClass c) const { return used_[static_cast<size_t>(c)]; }
  // Would `bytes` more fit?
  bool fits(size_t bytes) const { return used() + bytes <= limit_; }
  void add(GpuClass c, size_t bytes) { used_[static_cast<size_t>(c)] += bytes; }
  void remove(GpuClass c, size_t bytes) {
    size_t &v = used_[static_cast<size_t>(c)];
    v = bytes > v ? 0 : v - bytes;
  }
  // Allocations admitted over the limit (visible work that could not wait).
  size_t overruns = 0;

 private:
  size_t limit_ = 96u * 1024 * 1024;
  size_t used_[static_cast<size_t>(GpuClass::Count)] = {};
};

// The app's GPU budget (one per process).
GpuBudget &gpu_budget();

// Estimated GPU bytes for a w×h RGBA texture. Drivers pad to tiles; count
// 16-pixel tiles so the estimate errs high (study M10).
inline size_t gpu_bytes(int w, int h) {
  size_t tw = (static_cast<size_t>(w > 0 ? w : 0) + 15) / 16 * 16;
  size_t th = (static_cast<size_t>(h > 0 ? h : 0) + 15) / 16 * 16;
  return tw * th * 4;
}

// The rendered-frame counter, advanced once per presented frame. Caches age
// resources in frames, not milliseconds (study M7, M8).
long long current_frame();
void advance_frame();

// CPU memory domains (decoded pixels in flight, decoded sources), in bytes.
struct CpuDomains {
  std::atomic<long long> decoded_sources{0};
  std::atomic<long long> pixels_in_flight{0};
};
CpuDomains &cpu_domains();

// What eviction needs to know about one resource.
struct Evictable {
  bool resident = false;
  int pins = 0;
  Relevance relevance = Relevance::Visible;
  long long last_frame = 0;
  size_t bytes = 0;
};

// The resource to evict next, or "" when nothing may go: resident, unpinned,
// not drawn in the last `min_age` frames; least relevant first, then oldest.
template <typename Map>
std::string pick_victim(const Map &entries, long long frame, long long min_age) {
  const std::string *best = nullptr;
  int best_rel = -1;
  long long best_frame = 0;
  for (const auto &kv : entries) {
    const Evictable &e = kv.second.ev;
    if (!e.resident || e.pins > 0 || frame - e.last_frame < min_age) continue;
    int rel = static_cast<int>(e.relevance);
    if (!best || rel > best_rel || (rel == best_rel && e.last_frame < best_frame)) {
      best = &kv.first;
      best_rel = rel;
      best_frame = e.last_frame;
    }
  }
  return best ? *best : std::string();
}

// Memory policy (study C8): Normal while browsing, Playback while the player is
// on top (a smaller budget: video decoding needs the memory), Background while
// another app is in front (every GPU handle released).
enum class MemoryPolicy { Normal, Playback, Background };
const char *memory_policy_name(MemoryPolicy p);
size_t budget_for(MemoryPolicy p);
