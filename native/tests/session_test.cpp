// Host unit tests for the native Buna session (no TV, no SDL).
#include <cstdio>
#include <string>
#include <vector>

#include "json.h"
#include "session.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

struct FakePort : PlayerPort {
  std::vector<std::string> calls;
  void play() override { calls.push_back("play"); }
  void pause() override { calls.push_back("pause"); }
  void seek(long long ms) override { calls.push_back("seek:" + std::to_string(ms)); }
  void select_track(const std::string &type, int index) override { calls.push_back(type + ":" + std::to_string(index)); }
  void set_subtitles(bool enabled) override { calls.push_back(enabled ? "subs:on" : "subs:off"); }
  int seeks() const {
    int n = 0;
    for (auto &c : calls) n += c.rfind("seek:", 0) == 0;
    return n;
  }
};

static MediaInfo info_with(long long duration) {
  MediaInfo info;
  info.duration_ms = duration;
  info.seekable = true;
  return info;
}

static void start_playing(Session &s, FakePort &port, long long &t) {
  s.open(&port, t);
  MediaInfo info = info_with(600000);
  s.on_event({MediaEvent::Loaded}, info, t);
  s.on_event({MediaEvent::SourceInfo}, info, t);
  t += 400;
  s.on_event({MediaEvent::Playing}, info, t);
  s.on_event({MediaEvent::Time, 0}, info, t);
}

static void test_startup() {
  FakePort port;
  Session s;
  long long t = 1000;
  s.open(&port, t);
  CHECK(s.phase() == Phase::Loading);
  s.on_event({MediaEvent::Loaded}, info_with(0), t);
  s.on_event({MediaEvent::Loaded}, info_with(0), t);  // duplicate loadCompleted
  CHECK(port.calls.size() == 1 && port.calls[0] == "play");
  t += 700;
  s.on_event({MediaEvent::Playing}, info_with(0), t);
  CHECK(s.phase() == Phase::Playing);
  CHECK(s.first_frame_ms() == 700);
}

static void test_clock_interpolates_but_caps() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  s.on_event({MediaEvent::Time, 5000}, info_with(600000), t);
  CHECK(s.position_ms(t + 500) == 5500);
  CHECK(s.position_ms(t + 9000) == 7000);  // never more than 2 s past the last report
  s.pause();
  s.on_event({MediaEvent::Paused}, info_with(600000), t + 100);
  CHECK(s.position_ms(t + 9000) == 5000);
}

static void test_seek_preview_commit_ack() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  MediaInfo info = info_with(600000);
  s.on_event({MediaEvent::Time, 10000}, info, t);
  s.nudge(1, t);
  s.nudge(1, t + 100);
  CHECK(s.preview_ms() && *s.preview_ms() == 30000);
  CHECK(port.seeks() == 0);
  s.tick(t + 450);  // the commit waits 400 ms after the last press
  CHECK(port.seeks() == 0);
  s.tick(t + 501);
  CHECK(port.seeks() == 1 && port.calls.back() == "seek:30000");
  CHECK(s.phase() == Phase::Seeking);
  CHECK(s.display_ms(t + 800) == 30000);
  // A stale report from before the seek must not move the clock.
  s.on_event({MediaEvent::Time, 10500}, info, t + 900);
  CHECK(s.pending_ms().has_value());
  s.on_event({MediaEvent::SeekDone}, info, t + 1200);
  CHECK(!s.pending_ms().has_value());
  CHECK(s.position_ms(t + 1200) == 30000);
  CHECK(s.phase() == Phase::Playing);
}

static void test_seek_ack_from_time_report() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  s.seek_to(120000, t);
  s.on_event({MediaEvent::Time, 119500}, info_with(600000), t + 500);
  CHECK(!s.pending_ms().has_value());
  CHECK(s.position_ms(t + 500) == 119500);
}

static void test_acceleration_and_clamp() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  s.on_event({MediaEvent::Time, 300000}, info_with(600000), t);
  std::vector<long long> steps;
  long long last = 300000;
  for (int i = 0; i < 10; i++) {
    s.nudge(1, t + i * 50);
    steps.push_back(*s.preview_ms() - last);
    last = *s.preview_ms();
  }
  CHECK(steps[0] == 10000 && steps[3] == 10000 && steps[4] == 30000 && steps[8] == 60000);
  s.cancel_preview();
  s.seek_to(10000000, t);
  CHECK(*s.pending_ms() == 599000);
  s.seek_to(-50, t);
  CHECK(*s.pending_ms() == 0);
}

static void test_stall_and_recovery() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  MediaInfo info = info_with(600000);
  s.on_event({MediaEvent::Time, 1000}, info, t);
  s.tick(t + 3500);
  CHECK(s.phase() == Phase::Buffering);
  s.on_event({MediaEvent::Time, 1400}, info, t + 3600);
  CHECK(s.phase() == Phase::Playing);
}

static void test_session_stats() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);  // t = 400
  MediaInfo info = info_with(600000);
  s.on_event({MediaEvent::Time, 1000}, info, 1400);
  s.tick(4900);                                   // no progress for 3.5 s: a stall begins
  s.on_event({MediaEvent::Time, 1400}, info, 8900);  // and ends 4 s later
  s.account(8900);
  const SessionStats &st = s.stats();
  CHECK(st.stalls == 1);
  CHECK(st.stall_ms == 4000);
  CHECK(st.watched_media_ms == 1400);
  CHECK(st.play_wall_ms == 8500);
  CHECK(st.first_frame_ms == 400);
  // A seek's own wait is not a stall.
  s.seek_to(60000, 9000);
  s.tick(14000);
  s.on_event({MediaEvent::SeekDone}, info, 14000);
  s.account(14000);
  CHECK(s.stats().stalls == 1);
  CHECK(s.stats().seeks == 1 && s.stats().seeks_landed == 1);
  SessionStats total;
  total += s.stats();
  total += s.stats();
  CHECK(total.stalls == 2 && total.first_frame_ms == 400);
}

static void test_errors_and_timeouts() {
  FakePort port;
  Session s;
  long long t = 0;
  s.open(&port, t);
  s.tick(t + Session::kStartupTimeoutMs + 1);
  CHECK(s.phase() == Phase::Error);
  Session s2;
  start_playing(s2, port, t);
  s2.on_event({MediaEvent::Error, 0, "Decoder failure", 7}, info_with(1), t);
  CHECK(s2.phase() == Phase::Error && s2.error() == "Decoder failure");
  size_t before = port.calls.size();
  s2.play();
  CHECK(port.calls.size() == before);  // commands are ignored in error state
}

static void test_tracks() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  s.select_audio(1);
  CHECK(port.calls.back() == "audio:1");
  s.select_text(0);
  CHECK(port.calls[port.calls.size() - 2] == "text:0" && port.calls.back() == "subs:on");
  s.select_text(-1);
  CHECK(port.calls.back() == "subs:off" && s.subtitle_index() == -1);
}

static void test_end_and_replay() {
  FakePort port;
  Session s;
  long long t = 0;
  start_playing(s, port, t);
  s.on_event({MediaEvent::EndOfStream}, info_with(600000), t);
  CHECK(s.phase() == Phase::Ended);
  s.toggle();
  CHECK(port.calls.back() == "play");
}

static void test_json() {
  bool ok = false;
  Json j = Json::parse(R"({"sourceInfo":{"programInfo":[{"duration":1295552,"audioTrackInfo":[{"language":"en","codec":"ac3","channels":6}]}]},"t":"aé\n"})", &ok);
  CHECK(ok);
  CHECK(j["sourceInfo"]["programInfo"][0]["duration"].as_int() == 1295552);
  CHECK(j["sourceInfo"]["programInfo"][0]["audioTrackInfo"][0]["channels"].as_int() == 6);
  CHECK(j["t"].as_string() == "a\xC3\xA9\n");
  CHECK(j["missing"]["deeper"].as_string("x") == "x");
  Json::parse("{bad", &ok);
  CHECK(!ok);
  CHECK(Json::quote("a\"b\\c\n") == "\"a\\\"b\\\\c\\n\"");
}

int run_play_tests();
int run_runtime_tests();

int main() {
  test_startup();
  test_clock_interpolates_but_caps();
  test_seek_preview_commit_ack();
  test_seek_ack_from_time_report();
  test_acceleration_and_clamp();
  test_stall_and_recovery();
  test_session_stats();
  test_errors_and_timeouts();
  test_tracks();
  test_end_and_replay();
  test_json();
  failures += run_play_tests();
  failures += run_runtime_tests();
  if (failures) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("native session tests passed\n");
  return 0;
}
