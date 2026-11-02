// Bloob 4.0's pure parts: skip segments (skips.h).
#include <cmath>
#include <cstdio>
#include <string>

#include "ass.h"
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

static const char *kHeader =
    "[Script Info]\nScriptType: v4.00+\nPlayResX: 1920\nPlayResY: 1080\n\n"
    "[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, "
    "Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
    "Style: Default,Arial,72,&H00FFFFFF,&H000000FF,&H00000000,&H80000000,0,0,0,0,100,100,0,0,1,3,1,2,40,40,50,1\n"
    "Style: Sign,Arial,48,&H0000FFFF,&H000000FF,&H00402010,&H00000000,-1,0,0,0,100,100,0,0,1,2,0,8,10,10,30,1\n";

static void test_ass_header() {
  ass::Script sc = ass::parse_header(kHeader);
  CHECK(sc.valid && sc.res_x == 1920 && sc.res_y == 1080);
  CHECK(sc.styles.size() == 2);
  const ass::Style &sign = sc.style("Sign");
  CHECK(sign.bold && sign.align == 8 && sign.size == 48);
  CHECK((sign.primary == ass::Rgba{255, 255, 0, 255}));  // &H0000FFFF: yellow
  CHECK((sign.outline == ass::Rgba{0x10, 0x20, 0x40, 255}));
  CHECK((sc.style("Default").back == ass::Rgba{0, 0, 0, 127}));
  CHECK(sc.style("Missing").name == "Default");
  // No play resolution: libass's 384x288; only a width: 4:3.
  CHECK(ass::parse_header("[Script Info]\n").res_x == 384);
  CHECK(ass::parse_header("[Script Info]\nPlayResX: 640\n").res_y == 480);
  // SSA v4 alignment numbers (10 = middle centre).
  ass::Script old = ass::parse_header("[V4 Styles]\nFormat: Name, Fontsize, Alignment\nStyle: Default,20,10\n");
  CHECK(old.style("Default").align == 5);
}

static void test_ass_event() {
  ass::Script sc = ass::parse_header(kHeader);
  ass::Event e = ass::parse_event(sc, "3,0,Default,,0,0,0,,{\\i1}Hello{\\i0}, \\Nthere{\\c&H0000FF&\\b1} friend");
  CHECK(e.lines.size() == 2);
  CHECK(e.lines[0].size() == 2 && e.lines[0][0].text == "Hello" && e.lines[0][0].look.italic && !e.lines[0][1].look.italic);
  CHECK(e.lines[0][1].text == ", ");
  CHECK(e.lines[1].size() == 2 && e.lines[1][1].look.bold && (e.lines[1][1].look.primary == ass::Rgba{255, 0, 0, 255}));
  ass::Event sign = ass::parse_event(sc, "4,1,Sign,,0,0,0,,{\\an7\\pos(100,200)\\fad(200,300)\\fs60\\bord0}Tokyo");
  CHECK(sign.layer == 1 && sign.align == 7 && sign.has_pos && sign.x == 100 && sign.y == 200);
  CHECK(sign.fade_in == 200 && sign.fade_out == 300);
  CHECK(sign.lines[0][0].look.size == 60 && sign.lines[0][0].look.outline_w == 0);
  // Drawings are skipped; transforms, karaoke and comments are read but not shown.
  ass::Event draw = ass::parse_event(sc, "5,0,Default,,0,0,0,,{\\p1}m 0 0 l 100 0 100 100{\\p0}");
  CHECK(draw.empty());
  ass::Event k = ass::parse_event(sc, "6,0,Default,,0,0,0,,{\\k20}Ka{\\k30}ra{\\t(0,500,\\fs80)}oke{a note}");
  CHECK(k.lines[0].size() == 3 && k.lines[0][2].text == "oke" && k.lines[0][2].look.size == 72);
  // An empty \c goes back to the style's colour.
  ass::Event back = ass::parse_event(sc, "8,0,Default,,0,0,0,,{\\c&H0000FF&}red{\\c}white");
  CHECK((back.lines[0][1].look.primary == ass::Rgba{255, 255, 255, 255}));
  ass::Event mv = ass::parse_event(sc, "7,0,Default,,0,0,0,,{\\move(0,0,100,50)}Away");
  CHECK(mv.has_move && mv.x2 == 100 && mv.y2 == 50);
}

static void test_ass_layout() {
  ass::Script sc = ass::parse_header(kHeader);
  auto measure = [](const std::string &t, int size, bool) { return static_cast<int>(t.size()) * size / 2; };
  auto line_h = [](int size) { return size + size / 5; };
  // Bottom centre, margins 50 at 1920x1080 → 1:1.
  ass::Event e = ass::parse_event(sc, "1,0,Default,,0,0,0,,Hi there");
  ass::Layout l = ass::layout(sc, e, 1920, 1080, 500, 2000, measure, line_h);
  CHECK(l.runs.size() == 1);
  CHECK(l.runs[0].size == 72 && l.runs[0].width == 8 * 36);
  CHECK(std::abs(l.runs[0].x - (40 + (1920 - 80) / 2.0f - 144)) < 1);
  CHECK(std::abs(l.bottom - (1080 - 50)) < 1);
  // The same on a 1280x720 screen: everything two thirds.
  ass::Layout small = ass::layout(sc, e, 1280, 720, 500, 2000, measure, line_h);
  CHECK(small.runs[0].size == 48);
  // Top-left at \pos, faded in half way.
  ass::Event sign = ass::parse_event(sc, "2,0,Sign,,0,0,0,,{\\an7\\pos(100,200)\\fad(200,0)}Tokyo");
  ass::Layout s = ass::layout(sc, sign, 1920, 1080, 100, 3000, measure, line_h);
  CHECK(s.positioned && std::abs(s.runs[0].x - 100) < 1 && std::abs(s.top - 200) < 1);
  CHECK(s.runs[0].fill.a >= 126 && s.runs[0].fill.a <= 128);
  // A long line wraps inside the margins.
  std::string longer(60, 'a');
  ass::Event wide = ass::parse_event(sc, "3,0,Default,,0,0,0,," + longer + " " + longer);
  ass::Layout wl = ass::layout(sc, wide, 1920, 1080, 0, 1000, measure, line_h);
  CHECK(wl.runs.size() == 2 && wl.runs[1].y > wl.runs[0].y);
  // \move half way along.
  ass::Event mv = ass::parse_event(sc, "4,0,Sign,,0,0,0,,{\\an7\\move(0,0,200,100,0,1000)}Go");
  ass::Layout ml = ass::layout(sc, mv, 1920, 1080, 500, 500, measure, line_h);
  CHECK(std::abs(ml.runs[0].x - 100) < 1 && std::abs(ml.top - 50) < 1);
  // Two speakers at once: the second goes above the first.
  std::vector<ass::Layout> both = {l, ass::layout(sc, ass::parse_event(sc, "5,0,Default,,0,0,0,,Other"), 1920, 1080, 0, 1000, measure, line_h)};
  ass::stack(both, 1080);
  CHECK(both[1].bottom <= both[0].top + 0.5f);
}

int run_v4_tests() {
  test_chapter_titles();
  test_merge();
  test_offer();
  test_ass_header();
  test_ass_event();
  test_ass_layout();
  if (!failures) std::printf("4.0 tests passed\n");
  return failures;
}
