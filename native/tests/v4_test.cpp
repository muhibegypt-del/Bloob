// Bloob 4.0's pure parts: skip segments (skips.h).
#include <cstdio>
#include <string>

#include "skips.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_chapter_titles() {
  skips::Kind k;
  CHECK(skips::kind_of_title("Intro", &k) && k == skips::Kind::Intro);
  CHECK(skips::kind_of_title("Opening Credits", &k) && k == skips::Kind::Intro);
  CHECK(skips::kind_of_title("OP", &k) && k == skips::Kind::Intro);
  CHECK(skips::kind_of_title("Previously On...", &k) && k == skips::Kind::Recap);
  CHECK(skips::kind_of_title("End Credits", &k) && k == skips::Kind::Credits);
  CHECK(skips::kind_of_title("ED", &k) && k == skips::Kind::Credits);
  CHECK(!skips::kind_of_title("Chapter 3", &k));
  CHECK(!skips::kind_of_title("Opening Scene", &k));
  CHECK(!skips::kind_of_title("The Opera", &k));
  CHECK(!skips::kind_of_title("", &k));
}

static void test_merge() {
  const long long dur = 2900000;  // 48 min 20 s
  std::vector<skips::Chapter> ch = {{0, 60000, "Prologue"}, {60000, 120000, "Opening"}, {120000, 2820000, "Episode"},
                                    {2820000, 2900000, "End Credits"}};
  auto from_file = skips::from_chapters(ch, dur);
  CHECK(from_file.size() == 2);
  CHECK(from_file[1].kind == skips::Kind::Credits && from_file[1].end_ms == -1);
  std::vector<skips::Segment> intro_db = {{skips::Kind::Intro, 314500, 331000, "IntroDB"},
                                          {skips::Kind::Recap, 0, 40000, "IntroDB"},
                                          {skips::Kind::Credits, 2843000, 2901000, "IntroDB"}};
  auto merged = skips::merge({from_file, intro_db}, dur);
  // The file's chapters win for intro and credits; the recap comes from IntroDB.
  CHECK(merged.size() == 3);
  CHECK(merged[0].kind == skips::Kind::Recap && merged[0].source == "IntroDB");
  CHECK(merged[1].kind == skips::Kind::Intro && merged[1].source == "chapters" && merged[1].start_ms == 60000);
  CHECK(merged[2].kind == skips::Kind::Credits && merged[2].source == "chapters");
  // Implausible ones fall through to the next source: an intro made for a longer cut, credits at the start.
  std::vector<skips::Segment> wrong = {{skips::Kind::Intro, 3100000, 3150000, "TheIntroDB"},
                                       {skips::Kind::Credits, 10000, 20000, "TheIntroDB"}};
  auto m2 = skips::merge({wrong, intro_db}, dur);
  CHECK(m2.size() == 3 && m2[1].source == "IntroDB" && m2[2].source == "IntroDB");
  // Too short, and an intro of ten minutes: refused.
  CHECK(skips::merge({{{skips::Kind::Intro, 1000, 4000, "x"}, {skips::Kind::Intro, 0, 600000, "x"}}}, dur).empty());
}

static void test_offer() {
  const long long dur = 2900000;
  std::vector<skips::Segment> segs = {{skips::Kind::Intro, 60000, 120000, "chapters"}, {skips::Kind::Credits, 2843000, -1, "x"}};
  CHECK(skips::at(segs, 59999, dur) == -1);
  CHECK(skips::at(segs, 60000, dur) == 0);
  CHECK(skips::at(segs, 116000, dur) == 0);
  CHECK(skips::at(segs, 117500, dur) == -1);  // under 3 s left: not worth a button
  CHECK(skips::at(segs, 2850000, dur) == 1);
  CHECK(skips::target(segs[0], dur) == 120000);
  CHECK(skips::target(segs[1], dur) == dur - 1000);
  CHECK(std::string(skips::label(skips::Kind::Recap)) == "Skip recap");
  skips::Kind k;
  CHECK(skips::parse_name("credits", &k) && k == skips::Kind::Credits && !skips::parse_name("outro", &k));
}

int run_v4_tests() {
  test_chapter_titles();
  test_merge();
  test_offer();
  if (!failures) std::printf("4.0 tests passed\n");
  return failures;
}
