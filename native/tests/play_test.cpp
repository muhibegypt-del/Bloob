// Host unit tests for the play decisions (what to play, from where).
#include <cstdio>

#include "play.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static Episode ep(int season, int number) {
  Episode e;
  e.season = season;
  e.episode = number;
  e.id = "tt1:" + std::to_string(season) + ":" + std::to_string(number);
  e.title = "E" + std::to_string(number);
  return e;
}

static Meta show() {
  Meta m;
  m.item.id = "tt1";
  m.item.type = "series";
  for (int s = 1; s <= 2; s++) {
    Season season;
    season.season = s;
    for (int n = 1; n <= 3; n++) season.episodes.push_back(ep(s, n));
    m.seasons.push_back(season);
  }
  Season specials;
  specials.season = 0;
  specials.episodes.push_back(ep(0, 1));
  m.seasons.push_back(specials);
  return m;
}

static Watch watch(long long pos, long long dur, bool watched, long long updated) {
  Watch w;
  w.position = pos;
  w.duration = dur;
  w.watched = watched;
  w.updated = updated;
  return w;
}

static void test_resume_point() {
  CHECK(play::resume_point(watch(29000, 3600000, false, 0)) == 0);
  CHECK(play::resume_point(watch(30000, 3600000, false, 0)) == 30000);
  CHECK(play::resume_point(watch(3456000, 3600000, false, 0)) == 0);  // 96%
  CHECK(play::resume_point(watch(600000, 3600000, true, 0)) == 0);
}

static void test_title_default() {
  Meta m = show();
  std::map<std::string, Watch> p;
  PlayRequest first = play::title_default(m, p);
  CHECK(first.video_id == "tt1:1:1" && first.resume_ms == 0);  // never the specials
  p["tt1:1:2"] = watch(600000, 2400000, false, 10);
  PlayRequest resume = play::title_default(m, p);
  CHECK(resume.video_id == "tt1:1:2" && resume.resume_ms == 600000);
  p["tt1:1:3"] = watch(2400000, 2400000, true, 20);  // finale watched last
  PlayRequest next = play::title_default(m, p);
  CHECK(next.video_id == "tt1:2:1" && next.resume_ms == 0);  // into the next season
  CHECK(next.next && next.next->video_id == "tt1:2:2");
}

static void test_continue_and_after() {
  Resume r;
  r.item.id = "tt1";
  r.kind = "next";
  NextEpisode n;
  n.video_id = "tt1:1:3";
  n.season = 1;
  n.episode = 3;
  r.next = n;
  r.position = 999999;
  PlayRequest req = play::continue_item(r);
  CHECK(req.video_id == "tt1:1:3" && req.resume_ms == 0);
  CHECK(!play::after(req).has_value());
  PlayRequest cur;
  cur.next = n;
  auto a = play::after(cur);
  CHECK(a && a->video_id == "tt1:1:3" && a->resume_ms == 0);
}

int run_play_tests() {
  test_resume_point();
  test_title_default();
  test_continue_and_after();
  return failures;
}
