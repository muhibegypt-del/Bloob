#include "model.h"

#include <map>
#include <cstdio>
#include <cstdlib>
#include <ctime>

static std::vector<std::string> strings(const Json &list) {
  std::vector<std::string> out;
  for (size_t i = 0; i < list.size(); i++) {
    std::string s = list[i].as_string();
    if (!s.empty()) out.push_back(s);
  }
  return out;
}

Item parse_item(const Json &j) {
  Item it;
  it.id = j["id"].as_string();
  it.type = j["type"].as_string("movie");
  it.name = j["name"].as_string();
  it.year = j["year"].as_string();
  it.runtime = j["runtime"].as_string();
  it.rating = j["rating"].as_string();
  it.description = j["description"].as_string();
  it.genres = strings(j["genres"]);
  for (size_t i = 0; i < j["scores"].size(); i++) {
    const Json &sc = j["scores"][i];
    it.scores.push_back({sc["text"].as_string(), sc["label"].as_string(), sc["logo"].as_string()});
  }
  it.tile = j["tile"].as_string();
  it.backdrop = j["backdrop"].as_string();
  it.logo = j["logo"].as_string();
  it.poster = j["poster"].as_string();
  return it;
}

Resume parse_resume(const Json &j) {
  Resume r;
  r.item = parse_item(j);
  r.kind = j["kind"].as_string("resume");
  r.reason = j["reason"].as_string();
  r.video_id = j["videoId"].as_string();
  r.episode_title = j["episodeTitle"].as_string();
  r.season = static_cast<int>(j["season"].as_int());
  r.episode = static_cast<int>(j["episode"].as_int());
  r.position = j["position"].as_int();
  r.duration = j["duration"].as_int();
  const Json &n = j["next"];
  if (n.is_object()) {
    NextEpisode next;
    next.video_id = n["videoId"].as_string();
    next.title = n["title"].as_string();
    next.season = static_cast<int>(n["season"].as_int());
    next.episode = static_cast<int>(n["episode"].as_int());
    next.released = n["released"].as_string();
    r.next = next;
  }
  return r;
}

Meta parse_meta(const Json &j) {
  Meta m;
  m.item = parse_item(j);
  m.cast = strings(j["cast"]);
  m.director = strings(j["director"]);
  m.released = j["released"].as_string();
  const Json &seasons = j["seasons"];
  for (size_t i = 0; i < seasons.size(); i++) {
    Season s;
    s.season = static_cast<int>(seasons[i]["season"].as_int());
    s.label = seasons[i]["label"].as_string();
    const Json &eps = seasons[i]["episodes"];
    for (size_t k = 0; k < eps.size(); k++) {
      Episode e;
      e.id = eps[k]["id"].as_string();
      e.title = eps[k]["title"].as_string();
      e.overview = eps[k]["overview"].as_string();
      e.released = eps[k]["released"].as_string();
      e.thumbnail = eps[k]["thumbnail"].as_string();
      e.season = static_cast<int>(eps[k]["season"].as_int());
      e.episode = static_cast<int>(eps[k]["episode"].as_int());
      s.episodes.push_back(e);
    }
    if (!s.episodes.empty()) m.seasons.push_back(s);
  }
  return m;
}

std::map<std::string, Watch> parse_progress(const Json &j) {
  std::map<std::string, Watch> out;
  for (auto &entry : j.items()) {
    Watch w;
    w.position = entry.second["position"].as_int();
    w.duration = entry.second["duration"].as_int();
    w.watched = entry.second["watched"].as_bool();
    w.updated = entry.second["updated"].as_int();
    out[entry.first] = w;
  }
  return out;
}

Target parse_target(const Json &j) {
  Target t;
  if (!j.is_object()) return t;
  t.action = j["action"].as_string();
  t.video_id = j["videoId"].as_string();
  t.position = j["position"].as_int();
  const Json &u = j["upcoming"];
  if (u.is_object()) {
    NextEpisode n;
    n.video_id = u["videoId"].as_string();
    n.title = u["title"].as_string();
    n.released = u["released"].as_string();
    n.season = static_cast<int>(u["season"].as_int());
    n.episode = static_cast<int>(u["episode"].as_int());
    t.upcoming = n;
  }
  if (j["again"].is_object()) t.again_id = j["again"]["videoId"].as_string();
  return t;
}

Intent parse_intent(const Json &j) {
  Intent i;
  if (!j.is_object()) return i;
  i.from = j["from"].as_string();
  i.resolution = j["resolution"].as_string();
  i.hdr = j["hdr"].as_string();
  i.atmos = j["atmos"].as_bool();
  i.pack = j["pack"].as_bool();
  return i;
}

static SourceOption parse_option(const Json &j) {
  SourceOption o;
  o.id = j["id"].as_string();
  o.headline = j["headline"].as_string();
  o.resolution = j["resolution"].as_string();
  o.picture = j["picture"].as_string();
  o.audio = j["audio"].as_string();
  o.size = j["size"].as_string();
  o.availability = j["availability"].as_string();
  o.facts = j["facts"].as_string();
  o.release = j["release"].as_string();
  o.notes = strings(j["notes"]);
  o.warnings = strings(j["warnings"]);
  o.cautions = strings(j["cautions"]);
  o.excluded = j["excluded"].as_bool();
  o.match = j["match"].as_string();
  o.pack = j["scope"].as_string() == "pack";
  o.family = j["family"].as_string();
  o.source = j["source"].as_string();
  o.group = j["group"].as_string();
  std::string dv = j["dolbyVision"].as_string();
  o.dolby_vision = dv == "p8" ? 8 : dv == "p5" ? 5 : 0;
  // The indexes that found it, by name ("Found by SeaDex, Nyaa").
  static const std::map<std::string, std::string> kNames = {
      {"torrentio", "Torrentio"}, {"torrentsdb", "TorrentsDB"}, {"stremthru", "StremThru"}, {"apibay", "The Pirate Bay"},
      {"eztv", "EZTV"}, {"knaben", "Knaben"}, {"seadex", "SeaDex"}, {"kitsu", "Torrentio (anime)"}, {"nyaa", "Nyaa"}};
  for (const auto &id : strings(j["sources"])) {
    auto it = kNames.find(id);
    o.sources.push_back(it != kNames.end() ? it->second : id);
  }
  // "120 seeders" leads with the number; anything else is unknown.
  const std::string &a = o.availability;
  if (!a.empty() && a[0] >= '0' && a[0] <= '9') o.seeders = std::atoi(a.c_str());
  o.cut = j["cut"].as_string();
  o.tier = j["tier"].as_string();
  o.tier_label = j["tierLabel"].as_string();
  o.line = j["line"].as_string();
  o.compare = j["compare"].as_string();
  o.picture_name = j["pictureName"].as_string();
  o.tags = strings(j["tags"]);
  o.respected = j["respected"].as_bool();
  o.collapsed = j["collapsed"].as_bool();
  o.dupes = static_cast<int>(j["dupes"].as_int(0));
  o.ready = j["ready"].is_null() ? -1 : j["ready"].as_bool() ? 1 : 0;
  o.score = j["score"].as_number(0);
  o.bitrate_mbps = j["bitrateMbps"].is_null() ? -1 : static_cast<int>(j["bitrateMbps"].as_int(-1));
  return o;
}

Ranking parse_ranking(const Json &j) {
  Ranking r;
  if (j["recommended"].is_object()) r.recommended = parse_option(j["recommended"]);
  r.why = j["why"].as_string();
  r.provider = j["provider"].as_string();
  r.total = static_cast<int>(j["total"].as_int());
  const Json &groups = j["groups"];
  for (size_t i = 0; i < groups.size(); i++) {
    SourceGroup g;
    g.label = groups[i]["label"].as_string();
    g.kind = groups[i]["kind"].as_string("resolution");
    for (size_t k = 0; k < groups[i]["items"].size(); k++) g.items.push_back(parse_option(groups[i]["items"][k]));
    g.more = static_cast<int>(groups[i]["more"].as_int(0));
    r.groups.push_back(g);
  }
  for (size_t i = 0; i < j["cuts"].size(); i++)
    r.cuts.push_back({j["cuts"][i]["name"].as_string(), static_cast<int>(j["cuts"][i]["count"].as_int(0))});
  return r;
}

std::optional<NextEpisode> next_episode(const Meta &meta, const std::string &video_id) {
  bool found = false;
  for (auto &season : meta.seasons) {
    if (season.season == 0) continue;  // specials are not "next"
    for (auto &e : season.episodes) {
      if (found) return NextEpisode{e.id, e.title, e.released, e.season, e.episode};
      if (e.id == video_id) found = true;
    }
  }
  return std::nullopt;
}

bool aired(const std::string &released, const std::string &today) {
  if (released.size() < 10) return true;
  return released.compare(0, 10, today) <= 0;
}

std::string today_iso() {
  std::time_t now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  char buf[16];
  std::strftime(buf, sizeof buf, "%Y-%m-%d", &local);
  return buf;
}

static long days_from_civil(int y, int m, int d) {  // days since 1970-01-01
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  long yoe = y - era * 400;
  long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

std::string air_day_text(const std::string &released, const std::string &today) {
  int y = 0, m = 0, d = 0, ty = 0, tm = 0, td = 0;
  if (std::sscanf(released.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3 || std::sscanf(today.c_str(), "%4d-%2d-%2d", &ty, &tm, &td) != 3)
    return "soon";
  long days = days_from_civil(y, m, d) - days_from_civil(ty, tm, td);
  if (days <= 0) return "today";
  if (days == 1) return "tomorrow";
  static const char *weekdays[] = {"Thursday", "Friday", "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday"};  // 1970-01-01 was a Thursday
  if (days < 7) return std::string("on ") + weekdays[((days_from_civil(y, m, d) % 7) + 7) % 7];
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return "on " + std::to_string(d) + " " + months[(m - 1) % 12];
}

std::string episode_code(int season, int episode) {
  return "S" + std::to_string(season) + " \xC2\xB7 E" + std::to_string(episode);
}

std::string PlayRequest::label() const {
  if (!item.series()) return "";
  std::string code = episode_code(season, episode);
  return episode_title.empty() ? code : code + " \xC2\xB7 " + episode_title;
}
