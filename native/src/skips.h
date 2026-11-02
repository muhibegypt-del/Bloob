#pragma once
// Skip intro, recap and credits (4.0). Where they are comes from the file's own
// chapters first ("Intro", "Opening", "Recap", "End Credits"…), then the
// service's community databases (tv-app/service/skips.js: TheIntroDB, IntroDB,
// and AniSkip for anime), one segment of each kind, first source that has a
// plausible one. Pure: the player asks at() each frame.
#include <string>
#include <vector>

namespace skips {

enum class Kind { Intro, Recap, Credits };

struct Segment {
  Kind kind = Kind::Intro;
  long long start_ms = 0;
  long long end_ms = -1;  // -1: to the end of the file
  std::string source;     // "chapters", "TheIntroDB", "IntroDB", "AniSkip"
};

struct Chapter {
  long long start_ms = 0, end_ms = 0;
  std::string title;
};

// The kind a chapter title names, false when it names none.
bool kind_of_title(const std::string &title, Kind *kind);
// Segments from the file's chapters.
std::vector<Segment> from_chapters(const std::vector<Chapter> &chapters, long long duration_ms);
// One segment per kind: the first plausible one in `sources` (best first) for
// this file's length. Plausible: inside the file, at least 5 s long, an intro
// or recap at most 6 minutes and in the first 40%, credits in the last 40%.
std::vector<Segment> merge(const std::vector<std::vector<Segment>> &sources, long long duration_ms);
// The segment showing at `ms` that is worth offering (more than 3 s of it left), or -1.
int at(const std::vector<Segment> &segments, long long ms, long long duration_ms);
// Where a skip lands: the segment's end, or the end of the file less 1 s.
long long target(const Segment &s, long long duration_ms);
// "Skip intro", "Skip recap", "Skip credits".
const char *label(Kind kind);
// "intro", "recap", "credits" (the service's names), and back.
const char *name(Kind kind);
bool parse_name(const std::string &name, Kind *kind);

}  // namespace skips
