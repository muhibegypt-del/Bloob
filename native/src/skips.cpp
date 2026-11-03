#include "skips.h"

#include <algorithm>
#include <cctype>

namespace skips {
namespace {

std::string lower_words(const std::string &s) {
  // Lower case; anything not a letter or digit becomes one space.
  std::string out;
  for (unsigned char c : s) {
    if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
    else if (!out.empty() && out.back() != ' ') out += ' ';
  }
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

bool starts(const std::string &s, const char *p) {
  std::string w(p);
  return s.compare(0, w.size(), w) == 0 && (s.size() == w.size() || s[w.size()] == ' ');
}

bool plausible(const Segment &s, long long duration_ms) {
  if (s.start_ms < 0) return false;
  long long end = s.end_ms < 0 ? duration_ms : s.end_ms;
  if (duration_ms > 0) {
    if (s.start_ms >= duration_ms) return false;
    if (end > duration_ms + 5000) return false;  // made for a longer cut
  }
  if (end > 0 && end - s.start_ms < 5000) return false;
  if (s.kind != Kind::Credits) {
    if (end < 0 || end - s.start_ms > 6 * 60 * 1000) return false;
    if (duration_ms > 0 && s.start_ms > duration_ms * 4 / 10) return false;
  } else if (duration_ms > 0 && s.start_ms < duration_ms * 6 / 10) {
    return false;
  }
  return true;
}

}  // namespace

bool kind_of_title(const std::string &title, Kind *kind) {
  std::string t = lower_words(title);
  if (t.empty()) return false;
  static const char *const kIntro[] = {"intro", "introduction", "opening", "opening credits", "opening titles", "op",
                                       "title sequence", "main title", "main titles", "theme song", "titles"};
  static const char *const kRecap[] = {"recap", "previously", "previously on"};
  static const char *const kCredits[] = {"credits", "end credits", "ending", "ending credits", "closing credits",
                                         "end titles", "ed", "outro", "end"};
  for (const char *w : kIntro)
    if (t == w || (std::string(w).size() > 3 && starts(t, w) && t.find("scene") == std::string::npos)) return *kind = Kind::Intro, true;
  for (const char *w : kRecap)
    if (t == w || starts(t, w)) return *kind = Kind::Recap, true;
  for (const char *w : kCredits)
    if (t == w || (std::string(w).size() > 3 && starts(t, w))) return *kind = Kind::Credits, true;
  return false;
}

std::vector<Segment> from_chapters(const std::vector<Chapter> &chapters, long long duration_ms) {
  std::vector<Segment> out;
  for (size_t i = 0; i < chapters.size(); i++) {
    Kind k;
    if (!kind_of_title(chapters[i].title, &k)) continue;
    Segment s;
    s.kind = k;
    s.start_ms = chapters[i].start_ms;
    long long end = chapters[i].end_ms > chapters[i].start_ms ? chapters[i].end_ms
                    : i + 1 < chapters.size() ? chapters[i + 1].start_ms : -1;
    // Credits that run to the file's end stay open-ended.
    s.end_ms = k == Kind::Credits && (end < 0 || (duration_ms > 0 && end >= duration_ms - 1000)) ? -1 : end;
    s.source = "chapters";
    out.push_back(s);
  }
  return out;
}

std::vector<Segment> merge(const std::vector<std::vector<Segment>> &sources, long long duration_ms) {
  std::vector<Segment> out;
  for (Kind k : {Kind::Recap, Kind::Intro, Kind::Credits}) {
    for (const auto &list : sources) {
      auto it = std::find_if(list.begin(), list.end(), [&](const Segment &s) { return s.kind == k && plausible(s, duration_ms); });
      if (it != list.end()) {
        out.push_back(*it);
        break;
      }
    }
  }
  std::sort(out.begin(), out.end(), [](const Segment &a, const Segment &b) { return a.start_ms < b.start_ms; });
  return out;
}

long long target(const Segment &s, long long duration_ms) {
  if (s.end_ms >= 0) return s.end_ms;
  return duration_ms > 1000 ? duration_ms - 1000 : s.start_ms;
}

int at(const std::vector<Segment> &segments, long long ms, long long duration_ms) {
  for (size_t i = 0; i < segments.size(); i++) {
    const Segment &s = segments[i];
    long long end = s.end_ms >= 0 ? s.end_ms : duration_ms;
    if (ms >= s.start_ms && (end <= 0 || ms < end - 3000)) return static_cast<int>(i);
  }
  return -1;
}

const char *label(Kind kind) {
  return kind == Kind::Intro ? "Skip intro" : kind == Kind::Recap ? "Skip recap" : "Skip credits";
}

const char *name(Kind kind) { return kind == Kind::Intro ? "intro" : kind == Kind::Recap ? "recap" : "credits"; }

bool parse_name(const std::string &n, Kind *kind) {
  if (n == "intro") return *kind = Kind::Intro, true;
  if (n == "recap") return *kind = Kind::Recap, true;
  if (n == "credits") return *kind = Kind::Credits, true;
  return false;
}

}  // namespace skips
