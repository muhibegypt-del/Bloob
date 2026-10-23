#pragma once
// Data shown by the screens, parsed from the local service's JSON.
#include <map>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "json.h"

// A score on a title page: the site's logo (SVG via /api/image), "97%", and
// "Rotten Tomatoes 97%" for speech and when the logo cannot load.
struct Score {
  std::string text, label, logo;
};

struct Item {
  std::string id, type, name, year, runtime, rating, description;
  std::vector<Score> scores;  // title details only (MDBList): RT, Metacritic, IMDb, Letterboxd
  std::vector<std::string> genres;
  std::string tile, backdrop, logo, poster;
  bool series() const { return type == "series"; }
};

struct NextEpisode {
  std::string video_id, title;
  std::string released;  // ISO date from the catalogue; empty when unknown
  int season = 0, episode = 0;
};

// Continue watching: an item plus where it was left.
struct Resume {
  Item item;
  std::string kind;    // "resume", "next", or "upcoming" (caught up: no Play yet)
  std::string reason;  // "new" when the next episode aired since you finished
  std::string video_id, episode_title;
  int season = 0, episode = 0;
  long long position = 0, duration = 0;
  std::optional<NextEpisode> next;
};

struct Row {
  std::string id, title;
  std::vector<Item> items;
  std::vector<Resume> resume;  // set for the continue-watching row instead of items
  bool continue_row() const { return !resume.empty(); }
  size_t size() const { return continue_row() ? resume.size() : items.size(); }
};

struct Episode {
  std::string id, title, overview, released, thumbnail;
  int season = 0, episode = 0;
};

struct Season {
  int season = 0;
  std::string label;
  std::vector<Episode> episodes;
};

struct Meta {
  Item item;
  std::vector<std::string> cast, director;
  std::string released;
  std::vector<Season> seasons;
};

// What Play does for a title, decided by the service (UX-PLAN X2).
struct Target {
  std::string action;  // resume, next, start, caughtUp (series); play, again, resume (films)
  std::string video_id;
  long long position = 0;
  std::optional<NextEpisode> upcoming;  // caught up: the next episode and when it airs
  std::string again_id;                 // caught up: where watching again starts
  bool valid() const { return !action.empty(); }
};

// The version a show has been watched in (UX-PLAN X3): quality words only.
struct Intent {
  std::string from;  // "chosen" or "played"
  std::string resolution, hdr;
  bool atmos = false, pack = false;
  bool known() const { return !resolution.empty(); }
};

struct Watch {
  long long position = 0, duration = 0;
  bool watched = false;
  long long updated = 0;
};

struct SourceOption {
  std::string id, headline, resolution, picture, audio, size, availability, facts, release;
  std::vector<std::string> notes, warnings, cautions;  // cautions: short forms for the table
  bool excluded = false;
  std::string match;  // "same" or "like": continues the show's last version
  bool pack = false;  // part of a season pack (else a single episode file)
  std::string source, group;  // "REMUX", "BluRay", "WEB-DL"…; the release group
  std::string family;       // picture family: "DV", "HDR" or "SDR"
  int dolby_vision = 0;     // guessed Dolby Vision profile for the player's hint: 5, 8 or 0
  std::vector<std::string> sources;  // the indexes that listed it
  int seeders = -1;         // from the availability text; -1 unknown
  // 2.8, the cinephile list (service cinephile.js): cut ("" = theatrical), tier,
  // the one-line summary, how it compares with the best of its cut, and more.
  std::string cut, tier, tier_label, line, compare, picture_name;
  std::vector<std::string> tags;
  bool respected = false;   // a release group known for careful work
  bool collapsed = false;   // folded behind "more" until asked for
  int dupes = 0;            // the same release found again elsewhere
  int ready = -1;           // ready now on the owner's AllDebrid: 1, 0, or -1 unknown
  double score = 0;
  int bitrate_mbps = -1;    // the file's average, from size and running time; -1 unknown
};

struct SourceGroup {
  std::string label;
  std::string kind;  // "continue", "resolution" or "problems"
  std::vector<SourceOption> items;
  int more = 0;  // versions the service left out of this section
};

struct Ranking {
  std::optional<SourceOption> recommended;
  std::string why, provider;
  std::vector<SourceGroup> groups;
  int total = 0;
  std::vector<std::pair<std::string, int>> cuts;  // the edition row: name and count, "Theatrical" first
};

// What the viewer asked to watch.
struct PlayRequest {
  Item item;                 // the film or series
  std::string video_id;      // film ID or "tt…:season:episode"
  int season = 0, episode = 0;
  std::string episode_title, episode_thumbnail;
  long long resume_ms = 0;   // where to continue from (0 = start)
  std::string candidate;     // a version chosen on the title page (empty: the recommended one)
  std::optional<NextEpisode> next;
  std::string label() const;  // "S2 · E4 · The Crossing"
};

Item parse_item(const Json &j);
Resume parse_resume(const Json &j);
Meta parse_meta(const Json &j);
std::map<std::string, Watch> parse_progress(const Json &j);
Target parse_target(const Json &j);
Intent parse_intent(const Json &j);
Ranking parse_ranking(const Json &j);

// The episode after `video_id` in `meta`, if any (the next season's first after a
// finale), aired or not: check aired() before offering to play it.
std::optional<NextEpisode> next_episode(const Meta &meta, const std::string &video_id);
// Whether an ISO date is on or before `today` ("YYYY-MM-DD"); unknown dates count as aired.
bool aired(const std::string &released, const std::string &today);
std::string today_iso();
// "today", "tomorrow", "on Thursday" (within a week), else "on 22 Oct".
std::string air_day_text(const std::string &released, const std::string &today);
std::string episode_code(int season, int episode);  // "S2 · E4"
