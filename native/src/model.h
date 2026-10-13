#pragma once
// Data shown by the screens, parsed from the local service's JSON.
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "json.h"

struct Item {
  std::string id, type, name, year, runtime, rating, description;
  std::vector<std::string> genres;
  std::string tile, backdrop, logo, poster;
  bool series() const { return type == "series"; }
};

struct NextEpisode {
  std::string video_id, title;
  int season = 0, episode = 0;
};

// Continue watching: an item plus where it was left.
struct Resume {
  Item item;
  std::string kind;  // "resume" or "next"
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

struct Watch {
  long long position = 0, duration = 0;
  bool watched = false;
  long long updated = 0;
};

struct SourceOption {
  std::string id, headline, resolution, picture, audio, size, availability, facts, release;
  std::vector<std::string> notes, warnings, cautions;  // cautions: short forms for the table
  bool excluded = false;
};

struct SourceGroup {
  std::string label;
  std::vector<SourceOption> items;
};

struct Ranking {
  std::optional<SourceOption> recommended;
  std::string why, provider;
  std::vector<SourceGroup> groups;
  int total = 0;
};

// What the viewer asked to watch.
struct PlayRequest {
  Item item;                 // the film or series
  std::string video_id;      // film ID or "tt…:season:episode"
  int season = 0, episode = 0;
  std::string episode_title, episode_thumbnail;
  long long resume_ms = 0;   // where to continue from (0 = start)
  std::optional<NextEpisode> next;
  std::string label() const;  // "S2 · E4 · The Crossing"
};

Item parse_item(const Json &j);
Resume parse_resume(const Json &j);
Meta parse_meta(const Json &j);
std::map<std::string, Watch> parse_progress(const Json &j);
Ranking parse_ranking(const Json &j);

// The episode after `video_id` in `meta`, if any (the next season's first after a finale).
std::optional<NextEpisode> next_episode(const Meta &meta, const std::string &video_id);
std::string episode_code(int season, int episode);  // "S2 · E4"
