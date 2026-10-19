// Host unit tests for the runtime resource layer (R5a, R5b): frame pacing,
// the GPU budget and the eviction policy. Pure logic, no SDL.
#include <cstdio>
#include <map>
#include <string>

#include <atomic>
#include <memory>
#include <unistd.h>
#include <vector>

#include "frames.h"
#include "resources.h"
#include "tasks.h"
#include "stats.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static bool near(double a, double b) { return a > b - 0.0005 && a < b + 0.0005; }

static void test_pacer_idle_start() {
  FramePacer p;
  p.set_refresh_hz(60);
  // The first frame after idle steps one refresh, not the idle gap (the 2.3
  // loop stepped up to 100 ms here, so a focus move jumped on its first frame).
  CHECK(near(p.step_seconds(1000, false), 1.0 / 60));
  CHECK(near(p.step_seconds(5000, true), 1.0 / 60));  // a gap over 100 ms is idle too
}

static void test_pacer_median() {
  FramePacer p;
  p.set_refresh_hz(60);
  p.step_seconds(0, false);
  CHECK(near(p.step_seconds(17, true), 0.017));
  CHECK(near(p.step_seconds(34, true), 0.017));
  // One late frame: the median of the last three holds the step steady.
  CHECK(near(p.step_seconds(84, true), 0.017));
  CHECK(near(p.step_seconds(101, true), 0.017));
  // Steady slower frames are followed.
  p.step_seconds(141, true);
  p.step_seconds(181, true);
  CHECK(near(p.step_seconds(221, true), 0.040));
}

static void test_pacer_floor_and_short() {
  FramePacer p;
  p.set_refresh_hz(60);
  p.step_seconds(0, false);
  // Iterations closer than a refresh (input between frames) step their real time.
  CHECK(near(p.step_seconds(5, true), 0.005));
  // Never longer than 100 ms.
  p.step_seconds(100, true);
  p.step_seconds(199, true);
  CHECK(p.step_seconds(298, true) <= 0.1 + 1e-9);
}

static void test_pacer_due() {
  FramePacer p;
  p.set_refresh_hz(60);
  CHECK(p.due(0, false));  // nothing presented yet
  p.presented(1000);
  CHECK(!p.due(1005, false));
  CHECK(p.due(1005, true));  // input does not wait
  CHECK(p.due(1015, false));
  CHECK(p.wait_ms(1005) == 9);
  p.set_refresh_hz(0);  // unknown rate: 60 Hz
  CHECK(near(p.refresh_ms(), 1000.0 / 60));
}

static void test_budget() {
  GpuBudget b;
  b.set_limit(100);
  CHECK(b.fits(100));
  b.add(GpuClass::Surfaces, 40);
  b.add(GpuClass::Images, 50);
  CHECK(b.used() == 90);
  CHECK(!b.fits(11));
  CHECK(b.fits(10));
  b.remove(GpuClass::Images, 80);  // never below zero
  CHECK(b.used(GpuClass::Images) == 0);
  CHECK(b.used() == 40);
  CHECK(gpu_bytes(1, 1) == 16 * 16 * 4);
  CHECK(gpu_bytes(1920, 1080) == 1920u * 1088 * 4);
  CHECK(budget_for(MemoryPolicy::Playback) < budget_for(MemoryPolicy::Normal));
  CHECK(budget_for(MemoryPolicy::Background) < budget_for(MemoryPolicy::Playback));
}

struct Res {
  Evictable ev;
};

static void test_victim_order() {
  std::map<std::string, Res> m;
  auto add = [&m](const char *k, Relevance r, long long frame, int pins = 0, bool resident = true) {
    Res x;
    x.ev.relevance = r;
    x.ev.last_frame = frame;
    x.ev.pins = pins;
    x.ev.resident = resident;
    x.ev.bytes = 10;
    m[k] = x;
  };
  add("visible-old", Relevance::Visible, 10);
  add("near-new", Relevance::Near, 50);
  add("prefetch-new", Relevance::Prefetch, 90);
  add("prefetch-old", Relevance::Prefetch, 20);
  add("pinned", Relevance::Prefetch, 1, 1);
  add("gone", Relevance::Prefetch, 1, 0, false);
  add("just-drawn", Relevance::Prefetch, 99);
  // Least relevant first, then oldest; never pinned, non-resident or recent.
  CHECK(pick_victim(m, 100, 3) == "prefetch-old");
  m.erase("prefetch-old");
  CHECK(pick_victim(m, 100, 3) == "prefetch-new");
  m.erase("prefetch-new");
  CHECK(pick_victim(m, 100, 3) == "near-new");
  m.erase("near-new");
  CHECK(pick_victim(m, 100, 3) == "visible-old");
  m.erase("visible-old");
  CHECK(pick_victim(m, 100, 3).empty());  // only pinned, non-resident and just drawn remain
  CHECK(pick_victim(m, 103, 3) == "just-drawn");  // old enough three frames later
}

static void test_frame_counter() {
  long long a = current_frame();
  advance_frame();
  CHECK(current_frame() == a + 1);
}

// The 2.3 worker never woke the UI thread: it tested the completion after
// moving it into the queue. Every completion must now post exactly one wake.
static void test_tasks_wake_and_lanes() {
  std::atomic<int> wakes{0};
  Tasks t;
  t.start(1, [&] { wakes++; }, "bl-test");
  std::vector<int> order;
  // Block the single worker so the next jobs queue up, then release it.
  std::atomic<bool> go{false};
  t.run([&]() -> std::function<void()> {
    while (!go.load()) usleep(1000);
    return [&order] { order.push_back(0); };
  });
  usleep(20000);
  t.run([]() -> std::function<void()> { return nullptr; }, nullptr, 2, true);  // prefetch, no completion
  t.run([&]() -> std::function<void()> { return [&order] { order.push_back(2); }; }, nullptr, 2, true);
  t.run([&]() -> std::function<void()> { return [&order] { order.push_back(1); }; }, nullptr, 0, true);
  auto owner = std::make_shared<int>(0);
  t.run([&]() -> std::function<void()> { return [&order] { order.push_back(9); }; }, owner, 1, true);
  owner.reset();  // the owner is gone before the job runs: dropped
  go = true;
  for (int i = 0; i < 200 && t.pending(); i++) usleep(2000);
  usleep(10000);
  size_t ran = t.pump();
  CHECK(ran == 3);
  CHECK(wakes.load() == 3);  // one per completion, none for the job without one
  CHECK(order.size() == 3 && order[0] == 0 && order[1] == 1 && order[2] == 2);  // visible lane before prefetch
  Tasks::Stats st = t.stats();
  CHECK(st.dropped_owner == 1);
  t.stop();
}

static void test_tasks_pump_budget() {
  Tasks t;
  t.start(1, [] {}, "bl-test");
  for (int i = 0; i < 5; i++)
    t.run([]() -> std::function<void()> {
      return [] { usleep(3000); };
    });
  for (int i = 0; i < 200 && t.pending(); i++) usleep(2000);
  bool more = false;
  size_t ran = t.pump(4, &more);  // about two 3 ms completions fit in 4 ms
  CHECK(ran >= 1 && ran < 5);
  CHECK(more);
  t.pump(0, &more);
  CHECK(!more);
  t.stop();
}

static void test_stats_parsers() {
  const char *dev =
      "Inter-|   Receive                                                |  Transmit\n"
      " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets\n"
      "    lo: 5000000     100    0    0    0     0          0         0  5000000     100\n"
      " wlan0: 9000000000 7000    0    0    0     0          0         0  300000     900\n"
      "  eth0:     1200      10    0    0    0     0          0         0      0       0\n";
  std::string busiest;
  CHECK(stats::parse_net_rx(dev, &busiest) == 9000001200LL);  // loopback excluded
  CHECK(busiest == "wlan0");
  CHECK(stats::parse_net_rx("garbage") == -1);
  long long total = 0, idle = 0;
  CHECK(stats::parse_cpu_total("cpu  100 0 50 800 50 0 0 0 0 0\ncpu0 1 2 3\n", &total, &idle));
  CHECK(total == 1000 && idle == 850);
  CHECK(!stats::parse_cpu_total("intr 1 2", &total, &idle));
  // The command name may hold spaces and parentheses; utime 7, stime 3.
  CHECK(stats::parse_self_ticks("42 (bloob (main)) S 1 1 1 0 -1 4194304 10 0 0 0 7 3 0 0 20 0") == 10);
}

static void test_rate_meter() {
  stats::RateMeter m;
  CHECK(m.mbps() < 0);
  m.add(0, 0);
  m.add(1000, 12500000);  // 100 Mbit in one second
  CHECK(near(m.mbps(), 100.0));
  m.add(2000, 18750000);  // then 50 Mbit
  CHECK(near(m.mbps(), 75.0));
  CHECK(near(m.mbps(1000), 50.0));
  CHECK(near(m.average_mbps(), 75.0));
  CHECK(m.peak_mbps() >= 100.0 - 1e-6);
  CHECK(m.bytes() == 18750000);
  m.add(3000, 100);  // the counter went back (interface restarted): start again
  CHECK(m.bytes() == 0);
  CHECK(stats::mbps_text(58.43) == "58.4 Mbps");
  CHECK(stats::mbps_text(0.85) == "850 kbps");
  CHECK(stats::mbps_text(-1) == "\xE2\x80\x94");
  CHECK(stats::bytes_text(74200000000LL) == "74.2 GB");
}

int run_runtime_tests() {
  test_stats_parsers();
  test_rate_meter();
  test_tasks_wake_and_lanes();
  test_tasks_pump_budget();
  test_pacer_idle_start();
  test_pacer_median();
  test_pacer_floor_and_short();
  test_pacer_due();
  test_budget();
  test_victim_order();
  test_frame_counter();
  return failures;
}
