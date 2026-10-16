#pragma once
// Heap allocation counters (study M1, G3). Elements of A keeps small media
// metadata and per-frame geometry in arenas; Bloob first measures how many
// allocations a frame makes before deciding whether an arena would pay.
#include <atomic>

struct AllocStats {
  std::atomic<long long> count{0};
  std::atomic<long long> bytes{0};
};
AllocStats &alloc_stats();
